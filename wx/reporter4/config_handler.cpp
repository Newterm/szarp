
#include "config_handler.h"

#include "userreports.h"

ReportsConfigHandler::ReportsConfigHandler(const ServerInfoHolder& srv_info): srv_info(srv_info) {}

std::vector<ReportInfo> ReportsConfigHandler::GetSystemReports() const {
	return ParamsReportsHandler::GetReports(srv_info.base);
}

std::vector<ReportInfo> ReportsConfigHandler::GetUserReports() const {
	std::vector<ReportInfo> ret_vec;
	auto reports = UserReportsHandler::GetUserReports();
	for (auto it = reports.begin(); it != reports.end(); ++it) {
		if (it->config == srv_info.base) ret_vec.push_back(std::move(*it));
	}

	return ret_vec;
}

ReportInfo ReportsConfigHandler::LoadReportFromFile(const std::string& path) const {
	auto report = UserReportsHandler::LoadUserReport(path);
	if (report.config == srv_info.base) return report;
	else throw std::runtime_error("Report couldnt be loaded (source other than current config)");
}

void ReportsConfigHandler::SaveReport(const ReportInfo& report, const std::string& path) const {
	std::string real_path = path.empty()? UserReportsHandler::GetDefaultDirectory(): path;
	UserReportsHandler::SaveUserReport(report, real_path);
}

void ReportsConfigHandler::RemoveReport(const ReportInfo& report) const {
	UserReportsHandler::RemoveReport(report);
}

bool ReportsConfigHandler::UserReportAlreadyExists(const ReportInfo& report) const {
	return UserReportsHandler::UserReportAlreadyExists(report);
}

TSzarpConfig* ReportsConfigHandler::GetSzarpConfig() const {
	std::lock_guard<std::mutex> ipk_lock(ipk_mutex);
	if (ipk == nullptr) {
#ifdef MINGW32
		throw std::runtime_error("Accessing SZARP config not yet available on this platform.");
#else
		ipk = new TSzarpConfig();
		// TODO: get params from iks (not yet implemented)
		ipk->loadXML(L"/opt/szarp/"+srv_info.base+L"/config/params.xml", srv_info.base);
#endif
	}

	return ipk;
}

