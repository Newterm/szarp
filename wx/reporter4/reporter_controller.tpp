#ifndef __TPP_REPORTER__
#define __TPP_REPORTER__

#include <stdexcept>

#include "report_edit.h"

template<typename ConfigManager, typename DataProvider>
ReportController<ConfigManager, DataProvider>::ReportController(std::shared_ptr<ConfigManager> cfg, std::shared_ptr<DataProvider> dprov, const std::wstring& initial_report)
	: ReportControllerBase(), cfg(cfg), dprov(dprov), view(nullptr)

{
	view = new ReporterWindow(this);
	LoadSystemReports();
	LoadUserReports();

	if (initial_report != L"") {
		unsigned int r_no = 0;
		for (const auto& report: reports) {
			if (report.name == initial_report) {
				ShowReport(r_no);
				break;
			}

			++r_no;
		}
	}
}

template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::LoadSystemReports()
{
	auto system_reports = cfg->GetSystemReports();

	for (const ReportInfo& report: system_reports) {
		reports.push_back(report);
	}

	n_system_reports = system_reports.size();
	view->LoadSystemReports(system_reports);
}


template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::LoadUserReports() {
	reports.erase(reports.begin()+n_system_reports, reports.end());
	auto user_reports = cfg->GetUserReports();

	for (const ReportInfo& report: user_reports) {
		reports.push_back(report);
	}

	view->LoadUserReports(user_reports);
}


template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::ShowReport(unsigned int r_no) 
{
	if (r_no > reports.size()) {
		return;
	}

	ShowReport(reports[r_no]);
}

template<typename ConfigManager, typename DataProvider>
bool ReportController<ConfigManager, DataProvider>::SaveReport(const ReportInfo& report, std::string path) const
{
	if (cfg->UserReportAlreadyExists(report)) {
		if (view->AskOverwrite()) {
			cfg->SaveReport(report, path);
			return true;
		}
	} else {
		cfg->SaveReport(report, path);
		return true;
	}

	return false;
}


template<typename ConfigManager, typename DataProvider>
bool ReportController<ConfigManager, DataProvider>::SaveCurrentReport(std::string& path) const
{
	return SaveReport(current_report, path);
}

template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::LoadReportFromFile(const std::string& path) 
{
	try {
		auto report = cfg->LoadReportFromFile(path);

		ShowReport(report);
	} catch(const std::exception& e) {
		view->ShowError(e.what());
	}
}

template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::RemoveCurrentReport()
{
	view->Stop();
	cfg->RemoveReport(current_report);
	LoadUserReports();
}

template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::onValueUpdate(const std::wstring& param, const double val)
{
	view->UpdateValue(param, val);
}


template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::ShowReport(ReportInfo& report)
{
	current_report = report;
	view->ShowReport(report);
}

template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::OnStop() {
	dprov->Unsubscribe();
}

template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::OnStart() {
	if (current_report.name != L"") {
		dprov->SubscribeOnReport(current_report, this);
	}
}

template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::onSubscriptionError()
{
	view->onSubscriptionError();
	view->Stop();
}


template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::onSubscriptionStarted()
{
	view->onSubscriptionStarted();
}

template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::NewReport()
{
	ReportInfo report;
	auto ipk = cfg->GetSzarpConfig();

	report.name = L"Unnamed";
	report.config = ipk->GetPrefix();

	szReporterEdit ed(ipk, report, view, wxID_ANY);

	if (ed.ShowModal() != wxID_OK) {
		return;
	}

	while (!SaveReport(report)) {
		if (ed.ShowModal() == wxID_OK) {
			return;
		}
	}

	reports.push_back(report);
	LoadUserReports();

	ShowReport(report);
}

template<typename ConfigManager, typename DataProvider>
void ReportController<ConfigManager, DataProvider>::EditReport()
{
	ReportInfo report = current_report;
	auto ipk = cfg->GetSzarpConfig();

	szReporterEdit ed(ipk, report, view, wxID_ANY);

	if (ed.ShowModal() != wxID_OK) {
		return;
	}

	while (!SaveReport(report)) {
		if (ed.ShowModal() == wxID_OK) {
			return;
		}
	}

	reports.push_back(report);
	LoadUserReports();

	ShowReport(report);
}

#endif
