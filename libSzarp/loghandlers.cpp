#include "loghandlers.h"
#include "liblog.h"
#include <csignal>
#include <fstream>

namespace szlog {

void FileLogger::log_start(const std::string& filename) {
	/* std::signal(SIGABRT, SIG_IGN);
	std::signal(SIGINT, SIG_IGN);
	std::signal(SIGTERM, SIG_IGN); */

	std::ofstream file(filename, std::ofstream::out | std::ofstream::app);
	std::weak_ptr<Logger> m_logger = logger;

	while (true) {
		std::unique_lock<std::mutex> lk(_msg_mutex);
		if (m_logger.expired() || should_close) break;

		cv.wait(lk, [this]{ return !msg_q.empty(); });
		if (m_logger.expired() || should_close) break;

		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		auto localtime_t = std::localtime(&in_time_t);

		for (auto msg: msg_q) {
			file << format_date(localtime_t);
			file << " " << msg_priority_for_level(std::get<0>(msg)) << ": ";
			file << std::get<1>(msg);
			file << std::endl;
		}

		msg_q.clear();

		lk.unlock();
		cv.notify_all();
	}

	cv.notify_all();
}

void FileLogger::log(std::string&& msg, szlog::priority p) const {
	{
		std::lock_guard<std::mutex> lock(_msg_mutex);
		msg_q.push_back(std::tuple<szlog::priority, std::string>(p, msg));
	}

	cv.notify_one();

	{
		std::unique_lock<std::mutex> lock(_msg_mutex);
		cv.wait(lock, [this]{ return msg_q.empty() || should_close; }); // if passed lock, log was sent
	}
}

FileLogger::~FileLogger() {
	should_close = true;
	cv.notify_all();
	log("Closing logger", szlog::INFO);
}

} // namespace szlog
