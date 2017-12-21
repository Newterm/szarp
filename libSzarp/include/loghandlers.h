#ifndef _LIBLOGHANLDERS__H__
#define _LIBLOGHANLDERS__H__

#include <mutex>
#include <thread>
#include <iostream>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <list>

#ifndef __MINGW32__
#include <systemd/sd-journal.h>
#endif

namespace szlog {

enum str_mod { endl, flush, block };
enum priority: unsigned int { critical = 0, error = 2, warning = 4, info = 6, debug = 7 };

priority PriorityForLevel(int level);

std::string format_date(tm* localtime_t);

class LogHandler {
public:
	virtual void log(const std::string& msg, szlog::priority = szlog::priority::info) const = 0;
	virtual void log(const char* msg, szlog::priority = szlog::priority::info) const = 0;
	virtual ~LogHandler() {}
};

class COutLogger: public LogHandler {
public:
	void log(const std::string& msg, szlog::priority p = szlog::priority::info) const override;
	void log(const char* msg, szlog::priority p = szlog::priority::info) const override;

};

class JournaldLogger: public LogHandler {
	std::string name;

public:
	JournaldLogger(const std::string& _name): name(_name) {}

	void log(const std::string& msg, szlog::priority p = szlog::priority::info) const override;
	void log(const char* msg, szlog::priority p = szlog::priority::info) const override;

};

class FileLogger: public LogHandler {
public:
	FileLogger(std::string filename);

	void log(const std::string& msg, szlog::priority p = szlog::priority::info) const override;
	void log(const char* msg, szlog::priority p = szlog::priority::info) const override;

private:
	mutable std::unique_ptr<std::ofstream> logfile;
};

} // namespace szlog

#endif
