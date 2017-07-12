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
 * reporter4 program
 * SZARP

 * ecto@praterm.com.pl
 * pawel@praterm.com.pl
 */

#ifndef __REPORTER_CONTROLLER_H__
#define __REPORTER_CONTROLLER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "report.h"
#include "reporter_view.h"

#include "userreports.h"

class ReportControllerBase {
public:
	virtual ~ReportControllerBase() {}
	virtual void NewReport() = 0;
	virtual void EditReport() = 0;

	virtual void LoadSystemReports() = 0;
	virtual void LoadUserReports() = 0;
	virtual void LoadReportFromFile(const std::string& path) = 0;

	virtual bool SaveCurrentReport(std::string& path) const = 0; 
	virtual bool SaveReport(const ReportInfo& report, std::string path = "") const = 0;

	virtual void RemoveCurrentReport() = 0;

	virtual void ShowReport(ReportInfo&) = 0;
	virtual void ShowReport(unsigned int r_no) = 0;

	virtual void onSubscriptionError(const std::wstring&) = 0;
	virtual void onSubscriptionStarted() = 0;

	virtual void OnStop() = 0;
	virtual void OnStart() = 0;

	virtual void onValueUpdate(const std::wstring&, const double) = 0;

};


template <typename ConfigHandler, typename DataProvider, typename View>
class ReportController: public ReportControllerBase {
	std::shared_ptr<ConfigHandler> cfg;
	std::shared_ptr<DataProvider> dprov;
	View* view;

public:
	ReportController(std::shared_ptr<ConfigHandler> cfg, std::shared_ptr<DataProvider> dprov, View* view, const std::wstring& initial_report = L"");
	ReportController(const ReportController&) = delete;
	ReportController(ReportController&&) = delete;
	~ReportController() override {}

	virtual void NewReport() override;
	virtual void EditReport() override;

	void AddReport(ReportInfo& report);
	bool ReportEditor(ReportInfo& report);

	void LoadSystemReports() override;
	void LoadUserReports() override;
	void UpdateUserReports();
	void LoadReportFromFile(const std::string& path) override;

	bool SaveCurrentReport(std::string& path) const override;
	bool SaveReport(const ReportInfo& report, std::string path = "") const override;

	void RemoveCurrentReport() override;

	void ShowReport(ReportInfo&) override;
	void ShowReport(unsigned int r_no) override;

	void onValueUpdate(const std::wstring& param, const double val) override;

	void onSubscriptionError(const std::wstring& error_msg = L"") override;
	void onSubscriptionStarted() override;

	void OnStop() override;
	void OnStart() override;

private:
	std::vector<ReportInfo> reports;
	size_t n_system_reports;

	ReportInfo current_report;
};


#include "reporter_controller.hpp"

#endif //_REPORTER_CONTROLLER_H

