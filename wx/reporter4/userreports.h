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

#ifndef __USERREPORTS_H__
#define __USERREPORTS_H__

#include <vector>
#include <boost/filesystem.hpp>
#include "report.h"

class UserReportsHandler {
public:
	static std::string GetDefaultDirectory();

	static std::vector<ReportInfo> GetUserReports(const std::string& directory = GetDefaultDirectory());

	static void SaveUserReport(const ReportInfo& report, const std::string& path = GetDefaultDirectory());
	static ReportInfo LoadUserReport(const std::string& path = GetDefaultDirectory());

	static bool UserReportAlreadyExists(const ReportInfo& report, const std::string& path = GetDefaultDirectory());
	static void RemoveReport(const ReportInfo& report, const std::string& path = GetDefaultDirectory());

private:
	static void CreateDir(const boost::filesystem::path& directory);
};

class ParamsReportsHandler {
public:
	static std::vector<ReportInfo> GetReports(const std::wstring& config);

private:
	static const std::string GetPathForConfig(const std::string& config);

};

// TODO: add handling reports from iks


#endif	// __USERREPORTS_H__
