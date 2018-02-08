#ifndef __LIBLOG_H__
#define __LIBLOG_H__

#define loginit(level, logfile) \
     sz_loginit(level, logfile)

#define loginit_cmdline(leve, logfile, argc, argv) \
     sz_loginit_cmdline(leve, logfile, argc, argv) 

#define logdone() \
     sz_logdone()

#define log_info(info) \
     sz_log_info(info)

enum SZ_LIBLOG_FACILITY {
	SZ_LIBLOG_FACILITY_DAEMON,
	SZ_LIBLOG_FACILITY_APP 
};


#include "config.h"
#include <stdarg.h>
#include "loghandlers.h"
#include <mutex>
#include <map>
#include <thread>
#include <sstream>

#include <deque>
#include <future>

class ArgsManager;

// [[deprecated]]
int sz_loginit(int level, const char * logname, SZ_LIBLOG_FACILITY facility = SZ_LIBLOG_FACILITY_DAEMON, void *context = 0);

// [[deprecated]]
int sz_loginit_cmdline(int level, const char * logname, int *argc, char *argv[], SZ_LIBLOG_FACILITY facility = SZ_LIBLOG_FACILITY_DAEMON);

// [[deprecated]]
void sz_logdone(void);

// [[deprecated]]
void sz_log(int level, const char * msg_format, ...)
  __attribute__ ((format (printf, 2, 3)));

// [[deprecated]]
void vsz_log(int level, const char * msg_format, va_list fmt_args);

// [[deprecated]]
void sz_log_info(int info);


namespace szlog {

void init(const ArgsManager&, const std::string&);

struct LogEntry {
	std::stringstream _str;
	priority _p;

	LogEntry(const std::string& msg, priority p): _str(msg), _p(p) {}
};

class Logger {
	enum class DropPolicy { NODROP = false, DROP = true };
	DropPolicy drop_policy = DropPolicy::DROP;

	size_t max_log_msgs = 20;

	std::mutex _in_mutex;
	std::mutex _out_mutex;
	std::deque<std::shared_ptr<LogEntry>> _msgs_to_send;

	std::future<void> _msg_cv;
	bool logger_exited = true;
	std::function<void()> log_thread = [this](){ log_messages(); };

	std::mutex _msg_mutex;
	std::map<std::thread::id, std::shared_ptr<LogEntry>> _msgs;

	szlog::priority treshold = szlog::priority::error;
	std::shared_ptr<LogHandler> _logger;

	void log_messages();
	void log(const std::string& msg, priority p);

public:
	~Logger() {
		flush();
	}

	void flush();

	void log_later(std::shared_ptr<LogEntry> msg);
	void log_now(std::shared_ptr<LogEntry> msg);

	void parse_args(const ArgsManager& args_mgr);

	template <typename T, typename... Ts>
	void set_logger(Ts&&... args) {
		_logger = std::make_shared<T>(std::forward<Ts>(args)...);
	}

	template <typename T>
	void set_logger(std::shared_ptr<T> new_logger) {
		_logger = new_logger;
	}

	std::shared_ptr<LogHandler> get_logger() const {
		return _logger;
	}

	void set_log_treshold(szlog::priority p) {
		treshold = p;
	}

	szlog::priority get_log_treshold() {
		return treshold;
	}

	void set_log_treshold(int level) {
		set_log_treshold(PriorityForLevel(level));
	}

	template <typename T>
	Logger& operator<<(const T& msg);

};

template <typename T>
Logger& Logger::operator<<(const T& msg) {
	std::atomic_signal_fence(std::memory_order_relaxed);
	std::lock_guard<std::mutex> lock(_msg_mutex);
	const auto t_id = std::this_thread::get_id();
	_msgs[t_id]->_str << msg;
	return *this;
}

template <>
Logger& Logger::operator<<(const szlog::priority& p);

template <>
Logger& Logger::operator<<(const szlog::str_mod& m);

extern std::shared_ptr<Logger> logger;
Logger& log();

} // namespace szlog

#endif
