#ifndef _LIBLOGHANLDERS__H__
#define _LIBLOGHANLDERS__H__

#include <mutex>
#include <thread>
#include <iostream>
#include <atomic>
#include <condition_variable>
#include <list>

#ifndef MINGW32
#include <systemd/sd-journal.h>
#endif

namespace szlog {

enum str_mod { endl, flush, block };
enum priority: unsigned int { CRITICAL = 0, ERROR = 2, WARNING = 4, INFO = 6, DEBUG = 7 };

priority PriorityForLevel(int level);

const std::string msg_priority_for_level(szlog::priority p);

std::string format_date(tm* localtime_t);

class LogHandler {
public:
	virtual void log(const std::string& msg, szlog::priority = szlog::priority::INFO) const = 0;
	virtual void log(const char* msg, szlog::priority = szlog::priority::INFO) const = 0;
	virtual ~LogHandler() {}

	virtual void reinit() = 0;
};

class COutLogger: public LogHandler {
	mutable std::mutex _msg_mutex;

public:
	void log(const std::string& msg, szlog::priority p = szlog::priority::INFO) const override;
	void log(const char* msg, szlog::priority p = szlog::priority::INFO) const override;

	void reinit() override {
		_msg_mutex.unlock();
	}
};

class JournaldLogger: public LogHandler {
	mutable std::mutex _msg_mutex;
	std::string name;

public:
	JournaldLogger(const std::string& _name): name(_name) {}

	void log(const std::string& msg, szlog::priority p = szlog::priority::INFO) const override;
	void log(const char* msg, szlog::priority p = szlog::priority::INFO) const override;

	void reinit() override {
		_msg_mutex.unlock();
	}
};

class FileLogger: public LogHandler {
public:
	FileLogger(const std::string& filename);
	~FileLogger() override;

	void reinit() override;
	void log(const std::string& msg, szlog::priority p = szlog::priority::INFO) const override;
	void log(const char* msg, szlog::priority p = szlog::priority::INFO) const override;

private:
	mutable std::list<std::tuple<szlog::priority, std::string>> msg_q;
	mutable std::thread logger_thread;

	const std::string get_priority_prefix(szlog::priority p) const;

	void log_start(const std::string& filename);
	void processMessages(std::ofstream&);

	void blockSignals();
};

} // namespace szlog

#endif
