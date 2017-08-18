#ifndef __TPP_REPORTER__
#define __TPP_REPORTER__

#include <stdexcept>

#include "report_edit.h"

template<typename ConfigManager, typename DataProvider, typename View>
ReportController<ConfigManager, DataProvider, View>::ReportController(std::shared_ptr<ConfigManager> cfg, std::shared_ptr<DataProvider> dprov, View* view, const std::wstring& initial_report)
	: ReportControllerBase(), cfg(cfg), dprov(dprov), view(view)

{
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

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::LoadSystemReports()
{
	auto system_reports = cfg->GetSystemReports();

	for (const ReportInfo& report: system_reports) {
		reports.push_back(report);
	}

	n_system_reports = system_reports.size();
	view->LoadSystemReports(system_reports);
}


template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::LoadUserReports() {
	reports.erase(reports.begin()+n_system_reports, reports.end());
	auto user_reports = cfg->GetUserReports();

	for (const ReportInfo& report: user_reports) {
		reports.push_back(report);
	}

	view->LoadUserReports(user_reports);
}


template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::ShowReport(unsigned int r_no) 
{
	if (r_no > reports.size()) {
		return;
	}

	ShowReport(reports[r_no]);
}

template<typename ConfigManager, typename DataProvider, typename View>
bool ReportController<ConfigManager, DataProvider, View>::SaveReport(const ReportInfo& report, std::string path) const
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


template<typename ConfigManager, typename DataProvider, typename View>
bool ReportController<ConfigManager, DataProvider, View>::SaveCurrentReport(std::string& path) const
{
	return SaveReport(current_report, path);
}

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::LoadReportFromFile(const std::string& path) 
{
	try {
		auto report = cfg->LoadReportFromFile(path);

		ShowReport(report);
	} catch(const std::exception& e) {
		view->ShowError(e.what());
	}
}

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::UpdateUserReports() {
	const auto user_reports_size = reports.size() - n_system_reports;
	std::vector<ReportInfo> user_reports;
	user_reports.reserve(user_reports_size);

	for (unsigned int i = 0; i < user_reports_size; ++i) {
		user_reports.push_back(reports[i+n_system_reports]);
	}

	view->LoadUserReports(user_reports);
}

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::RemoveCurrentReport()
{
	view->Stop();

	const ReportInfo report_to_be_removed = current_report;
	auto e_it = std::find(reports.begin(), reports.end(), report_to_be_removed);
	if (e_it == reports.end()) return;

	reports.erase(e_it); 

	cfg->RemoveReport(report_to_be_removed);
	current_report = ReportInfo();

	UpdateUserReports();
}

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::onValueUpdate(const std::wstring& param, const double val)
{
	view->UpdateValue(param, val);
}


template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::ShowReport(ReportInfo& report)
{
	current_report = report;
	view->ShowReport(report);
}

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::OnStop() {
	dprov->Unsubscribe();
}

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::OnStart() {
	if (current_report.name != L"") {
		dprov->SubscribeOnReport(current_report, this);
	}
}

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::onSubscriptionError(const std::wstring& error_msg)
{
	view->onSubscriptionError(error_msg);
}


template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::onSubscriptionStarted()
{
	view->onSubscriptionStarted();
}

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::NewReport()
{
	view->Stop();

	ReportInfo report(L"Unnamed");
	if (!ReportEditor(report)) return;

	AddReport(report);
}

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::EditReport()
{
	view->Stop();

	ReportInfo report = current_report;
	if (!ReportEditor(report)) return;

	AddReport(report);
}

template<typename ConfigManager, typename DataProvider, typename View>
void ReportController<ConfigManager, DataProvider, View>::AddReport(ReportInfo& report) {
	for (auto it = reports.begin() + n_system_reports; it != reports.end(); ++it) {
		if (report.name == it->name) {
			*it = report;
			ShowReport(*it);
			return;
		}
	}

	reports.push_back(report);

	UpdateUserReports();
	ShowReport(report);
}

template<typename ConfigManager, typename DataProvider, typename View>
bool ReportController<ConfigManager, DataProvider, View>::ReportEditor(ReportInfo& report) {
	auto ipk = cfg->GetSzarpConfig();
	report.config = ipk->GetPrefix();

	szReporterEdit ed(ipk, report, view, wxID_ANY);

	if (ed.ShowModal() != wxID_OK) {
		return false;
	}

	while (!SaveReport(report)) {
		if (ed.ShowModal() != wxID_OK) {
			return false;
		}
	}

	return true;
}

#endif
