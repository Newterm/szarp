#ifndef __R4_CONFIG_HANDLER__
#define __R4_CONFIG_HANDLER__

#include <mutex>
#include <vector>

#include "../../libSzarp2/include/szarp_config.h"
#include "report.h"

class ReportsConfigHandler {
	const ServerInfoHolder srv_info;

	mutable std::mutex ipk_mutex;
	mutable TSzarpConfig* ipk{nullptr};

public:
	ReportsConfigHandler(const ServerInfoHolder& srv_info);

	std::vector<ReportInfo> GetSystemReports() const;
	std::vector<ReportInfo> GetUserReports() const;

	ReportInfo LoadReportFromFile(const std::string& path) const;

	void SaveReport(const ReportInfo& report, const std::string& path) const;

	void RemoveReport(const ReportInfo& report) const;

	bool UserReportAlreadyExists(const ReportInfo& report) const;

	TSzarpConfig* GetSzarpConfig() const;
};

#endif
