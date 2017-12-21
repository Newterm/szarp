#include "loghandlers.h"
#include "liblog.h"

namespace szlog {

void COutLogger::log(const std::string& msg, szlog::priority p) const {
	log(msg.c_str(), p);
}

void COutLogger::log(const char* msg, szlog::priority p) const {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	auto localtime_t = std::localtime(&in_time_t);

	std::cout << format_date(localtime_t) << " " << msg << std::endl;
}

void JournaldLogger::log(const std::string& msg, szlog::priority p) const {
	log(msg.c_str(), p);
}

void JournaldLogger::log(const char* msg, szlog::priority p) const {
#ifndef __MINGW32__
	sd_journal_send(
		"PRIORITY=%d", static_cast<std::underlying_type<szlog::priority>::type>(p),
		"SYSLOG_IDENTIFIER=%s", name.c_str(),
		"MESSAGE=%s", msg,
		NULL
	);
#endif
}

void FileLogger::log(const std::string& msg, szlog::priority p) const {
	log(msg.c_str(), p);
}

void FileLogger::log(const char* msg, szlog::priority p) const {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	auto localtime_t = std::localtime(&in_time_t);

	*logfile << format_date(localtime_t);
	*logfile << ": ";
	*logfile << msg;
	*logfile << std::endl;
}

FileLogger::FileLogger(std::string filename) {
#ifndef __MINGW32__
	// if not absolute path, force /var/log/szarp
	if (filename.front() != '/') {
		 filename = "/var/log/szarp/"+filename+".log";
	}
#endif

	// IN GCC > 5 use this one
	// logfile = std::move(std::ofstream(filename, std::ofstream::out | std::ofstream::app));

	logfile = std::unique_ptr<std::ofstream>(new std::ofstream(filename, std::ofstream::out | std::ofstream::app));
}

} // namespace szlog
