#ifndef _LIBLOGHANLDERS__H__
#define _LIBLOGHANLDERS__H__

#include <mutex>
#include <thread>
#include <iostream>
#include <atomic>
#include <condition_variable>
#include <list>

namespace szlog {

enum str_mod { endl, flush, block };
enum priority: unsigned int { CRITICAL = 0, ERROR = 2, WARNING = 4, INFO = 6, DEBUG = 7 };

priority PriorityForLevel(int level);

const std::string msg_priority_for_level(szlog::priority p);

std::string format_date(tm* localtime_t);

class LogHandler {
public:
	virtual void log(std::string&& msg, szlog::priority = szlog::priority::INFO) const = 0;
	virtual ~LogHandler() {}

};

class COutLogger: public LogHandler {
	mutable std::mutex _msg_mutex;

public:
	void log(std::string&& msg, szlog::priority p = szlog::priority::INFO) const override {
		std::lock_guard<std::mutex> lock(_msg_mutex);
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		auto localtime_t = std::localtime(&in_time_t);

		std::cout << format_date(localtime_t) << " " << msg_priority_for_level(p) << msg << std::endl;
	}

};

class JournaldLogger: public LogHandler {
	mutable std::mutex _msg_mutex;

	const std::string get_priority_prefix(szlog::priority p) const {
		return "<"+std::to_string(p)+">";
	}

public:
	void log(std::string&& msg, szlog::priority p = szlog::priority::INFO) const override {
		std::lock_guard<std::mutex> lock(_msg_mutex);
		std::cout << get_priority_prefix(p) << " " << msg << std::endl;
	}

};

class FileLogger: public LogHandler {
	mutable std::mutex _msg_mutex;
	std::atomic<bool> should_close{ false };
	
	mutable std::condition_variable cv;

	mutable std::list<std::tuple<szlog::priority, std::string>> msg_q;

	const std::string get_priority_prefix(szlog::priority p) const;

	void log_start(const std::string& filename);

public:
	FileLogger(const std::string& filename) {
		std::string logfile = filename;
		// if not absolute path, force /var/log/szarp
		if (filename.front() != '/') {
			logfile = "/var/log/szarp/"+filename+".log";
		}

		// check if passing logger is OK
		std::thread([this, logfile](){ log_start(logfile); }).detach();
	}

	~FileLogger() override;

	void log(std::string&& msg, szlog::priority p = szlog::priority::INFO) const override;

};

} // namespace szlog

#endif
