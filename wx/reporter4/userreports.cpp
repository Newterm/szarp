/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/* $Id$
 *
 * SZARP
 * Saving/loading/getting list of user reports
 */

#include <boost/range/iterator_range.hpp>

#include "userreports.h"
#include "conversion.h"
#include <stdexcept>

namespace fs = boost::filesystem;

fs::path FileNameForReport(const ReportInfo& report, const std::string& path) {
	if (fs::is_directory(path)) {
		return fs::path(path + SC::S2A(report.name) + ".xpl");
	} else {
		return fs::path(path);
	}
}


std::string UserReportsHandler::GetDefaultDirectory() {
	fs::path default_path(getenv("HOME"));
	default_path /= ".szarp";
	default_path /= "reports/";
	return default_path.string();
}

void UserReportsHandler::SaveUserReport(const ReportInfo& report, const std::string& path) {
	try {

		CreateDir(fs::path(path).parent_path());
		ReportParser::ToFile(FileNameForReport(report, path).string(), report);

	} catch(const std::exception& e) {
		std::cout << e.what() << std::endl;
		throw;
	}
}


ReportInfo UserReportsHandler::LoadUserReport(const std::string& path) {
	try {

		return ReportParser::FromFile(path);

	} catch(const std::exception& e) {
		std::cout << e.what() << std::endl;
		throw;
	}
}


bool UserReportsHandler::UserReportAlreadyExists(const ReportInfo& report, const std::string& path) {
	return fs::exists(FileNameForReport(report, path));
}

void UserReportsHandler::CreateDir(const fs::path& directory) {
	fs::path d_path(directory);
	auto status = fs::status(d_path);
	if (fs::is_directory(status)) {
		// assume writability
		return;
	} else if (!fs::create_directory(d_path)) {
		throw std::runtime_error("Could not create directory");
	}
}


void UserReportsHandler::RemoveReport(const ReportInfo& report, const std::string& path) {
	fs::remove(FileNameForReport(report, path));
}

std::vector<ReportInfo> UserReportsHandler::GetUserReports(const std::string& directory)
{
	std::vector<ReportInfo> reports{};

	fs::path d_path(directory);
	if (!fs::is_directory(d_path)) return reports;

	std::for_each(fs::directory_iterator(d_path), fs::directory_iterator{}, [&reports](fs::directory_entry const &entry) {
		if (fs::extension(entry) == ".xpl") {
			auto report_path = entry.path().string();

			try {

				reports.push_back(LoadUserReport(report_path));

			} catch (const std::exception& e) {
				std::cout << e.what() << std::endl;
			}
		}
	});
	
	return reports;
}


std::vector<ReportInfo> ParamsReportsHandler::GetReports(const std::wstring& config) {
	auto params_path = ParamsReportsHandler::GetPathForConfig(SC::S2A(config));

	try {

		return ReportParser::FromParams(params_path, config);

	} catch(const std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	return {};
}


const std::string ParamsReportsHandler::GetPathForConfig(const std::string& config) {
	return "/opt/szarp/" + config + "/config/params.xml";
}


