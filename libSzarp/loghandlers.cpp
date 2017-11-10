#include "loghandlers.h"
#include "liblog.h"
#include <csignal>
#include <fstream>
#include <csignal>

#include <pthread.h>

namespace szlog {

namespace {

// those have to be global in case of fork
std::mutex _filelogger_exit_mutex;

std::mutex _msg_mutex;
std::condition_variable cv;

std::atomic<bool> logger_should_exit{ false };

std::string logfile;

void prefork() {
	szlog::logger.reset();
}

void parent_postfork() {
}

void child_postfork() {
}

} // annon namespace

void COutLogger::log(const std::string& msg, szlog::priority p) const {
	log(msg.c_str(), p);
}


void COutLogger::log(const char* msg, szlog::priority p) const {
	std::atomic_signal_fence(std::memory_order_relaxed);
	std::lock_guard<std::mutex> lock(_msg_mutex);
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	auto localtime_t = std::localtime(&in_time_t);

	std::cout << format_date(localtime_t) << " " << msg_priority_for_level(p) << " " << msg << std::endl;
}

void JournaldLogger::log(const std::string& msg, szlog::priority p) const {
	log(msg.c_str(), p);
}

void JournaldLogger::log(const char* msg, szlog::priority p) const {
	std::atomic_signal_fence(std::memory_order_relaxed);
	std::lock_guard<std::mutex> lock(_msg_mutex);

#ifndef MINGW32
		sd_journal_send(
			"PRIORITY=%d", static_cast<std::underlying_type<szlog::priority>::type>(p),
			"SYSLOG_IDENTIFIER=%s", name.c_str(),
			"MESSAGE=%s", msg,
			NULL
		);
#else
		std::cout << "<" << static_cast<std::underlying_type<szlog::priority>::type>(p) << "> " << msg << std::endl;
#endif
}

void FileLogger::blockSignals() {
#ifndef _WIN32
	pthread_atfork(prefork, parent_postfork, child_postfork);

	// block all signals and close after destructor is called (e.g. exit handler)
	sigset_t mask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);
#endif
}

void FileLogger::processMessages(std::ofstream& file) {
#ifndef _WIN32
	if (!msg_q.empty()) {
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		auto localtime_t = std::localtime(&in_time_t);

		for (const auto& msg: msg_q) {
			file << format_date(localtime_t);
			file << " " << msg_priority_for_level(std::get<0>(msg)) << ": ";
			file << std::get<1>(msg);
			file << std::endl;
		}

		msg_q.clear();
	}
#endif
}

void FileLogger::log_start(const std::string& filename) {
#ifndef _WIN32
	std::lock_guard<std::mutex> exit_lock(_filelogger_exit_mutex);

	blockSignals();

	std::ofstream file(filename, std::ofstream::out | std::ofstream::app);

	while (!logger_should_exit) {
		std::unique_lock<std::mutex> lk(_msg_mutex);
		if (logger_should_exit) break;

		cv.wait(lk, [this]{ return !msg_q.empty() || logger_should_exit; });
		processMessages(file);

		cv.notify_all();
	}
#endif
}

void FileLogger::log(const std::string& msg, szlog::priority p) const {
#ifndef _WIN32
	std::atomic_signal_fence(std::memory_order_relaxed);
	{
		std::lock_guard<std::mutex> lock(_msg_mutex);
		std::string _msg = msg;
		msg_q.push_back(std::tuple<szlog::priority, std::string>(p, msg));
	}

	cv.notify_all();

	{
		std::unique_lock<std::mutex> lk(_msg_mutex);
		cv.wait(lk, [this]{ return msg_q.empty() || logger_should_exit; });
	}
#endif
}

void FileLogger::log(const char* msg, szlog::priority p) const {
#ifndef _WIN32
	log(std::string(msg));
#endif
}

FileLogger::FileLogger(const std::string& filename) {
#ifndef _WIN32
	// if not absolute path, force /var/log/szarp
	if (filename.front() != '/') {
		logfile = "/var/log/szarp/"+filename+".log";
	} else {
		logfile = filename;
	}

	logger_thread = std::thread([this](){ log_start(logfile); });
	logger_thread.detach();
#endif
}


FileLogger::~FileLogger() {
#ifndef _WIN32
	std::atomic_signal_fence(std::memory_order_relaxed);
	logger_should_exit = true;
	_msg_mutex.unlock();
	cv.notify_all();
	_filelogger_exit_mutex.lock();
#endif
}

void FileLogger::reinit() {
#ifndef _WIN32
	std::atomic_signal_fence(std::memory_order_relaxed);

	_msg_mutex.unlock();
	_msg_mutex.lock();
	cv.notify_all();
	_filelogger_exit_mutex.unlock();

	msg_q.clear();

	logger_thread = std::thread([this](){ log_start(logfile); });
	logger_thread.detach();

	_msg_mutex.unlock();
#endif
}

} // namespace szlog
