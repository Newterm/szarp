/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#ifndef MINGW32
#include <syslog.h>
#endif

#include "liblog.h"
#include "loghandlers.h"
#include "argsmgr.h"

namespace szlog {

std::shared_ptr<Logger> logger{new Logger()};

}

namespace {
std::mutex log_mutex;
}


int sz_loginit_cmdline(int level, const char * logfile, int *argc, char *argv[], SZ_LIBLOG_FACILITY)
{
	szlog::log().set_log_treshold(level);
#ifndef _WIN32
	if (logfile) {
		szlog::log().set_logger<szlog::JournaldLogger>(std::string(logfile));
	} else {
		szlog::log().set_logger<szlog::COutLogger>();
	}
#else
	szlog::log().set_logger<szlog::COutLogger>();
#endif

	return level;
}

namespace szlog {

void init(const ArgsManager& args_mgr, const std::string& logtag) {
	auto log_type = args_mgr.get<std::string>("logger");
#ifndef _WIN32
	if (log_type) {
		if (*log_type == "cout") {
			log().set_logger<COutLogger>();
		} else if (*log_type == "journald" || args_mgr.get<bool>("no-daemon").get_value_or(false) || args_mgr.get<bool>("no_daemon").get_value_or(false)) {
			log().set_logger<JournaldLogger>(logtag);
		} else if (*log_type == "file") {
			auto logfile_opt = args_mgr.get<std::string>("log_file").get_value_or(logtag);
			log().set_logger<FileLogger>(logfile_opt);
		} else {
			log().set_logger<COutLogger>();
		}
	} else {
		log().set_logger<JournaldLogger>(logtag);
	}
#else
	log().set_logger<COutLogger>();
#endif

	auto log_level = args_mgr.get<int>("log_level").get_value_or(2);

	// if debug is specified, use it
	log_level = args_mgr.get<unsigned int>("debug").get_value_or(log_level);

	log().set_log_treshold(log_level);
}

void Logger::log(std::shared_ptr<LogEntry> msg) {
	std::atomic_signal_fence(std::memory_order_relaxed);
	if (msg->_p > treshold) return;
	if (_logger)
		_logger->log(msg->_str.str(), msg->_p);
	else
		std::cout << msg->_str.str() << std::endl;
}

}

int sz_loginit(int level, const char * logname, SZ_LIBLOG_FACILITY, void *) {
#ifndef _WIN32
	if (logname != NULL) {
		auto pname = std::string(logname);
		szlog::log().set_logger<szlog::JournaldLogger>(pname);
	} else {
		szlog::log().set_logger<szlog::COutLogger>();
	}
#else
	szlog::log().set_logger<szlog::COutLogger>();
#endif

	szlog::log().set_log_treshold(level);
	return level;
}

void sz_logdone(void) {
}

void sz_log(int level, const char * msg_format, ...) {
	if (level > szlog::log().get_log_treshold()) return;

	va_list fmt_args;
	va_start(fmt_args, msg_format);
	vsz_log(level, msg_format, fmt_args);
	va_end(fmt_args);
}


void vsz_log(int level, const char * msg_format, va_list fmt_args) {
	std::atomic_signal_fence(std::memory_order_relaxed);
	std::lock_guard<std::mutex> lock(log_mutex);

	if (level > szlog::log().get_log_treshold()) return;

	char *l;
	if (vasprintf(&l, msg_format, fmt_args) != -1) {
		auto log_impl = szlog::log().get_logger();
		if (log_impl) {
			log_impl->log(l, szlog::PriorityForLevel(level));
		} else {
			szlog::log() << szlog::PriorityForLevel(level) << l << szlog::flush;
		}
	}

	if (l) free(l);
}

namespace szlog {

template <>
Logger& Logger::operator<<(const szlog::priority& p) {
	std::atomic_signal_fence(std::memory_order_relaxed);
	std::lock_guard<std::mutex> lock(_msg_mutex);
	const auto t_id = std::this_thread::get_id();

	if (!_msgs[t_id]) _msgs[t_id] = std::make_shared<LogEntry>();
	_msgs[t_id]->_p = p;
	return *this;
}

template <>
Logger& Logger::operator<<(const szlog::str_mod& m) {
	std::atomic_signal_fence(std::memory_order_relaxed);
	std::lock_guard<std::mutex> lock(_msg_mutex);
	const auto t_id = std::this_thread::get_id();

	if (!_msgs[t_id]) _msgs[t_id] = std::make_shared<LogEntry>();

	switch(m) {
	case szlog::str_mod::endl:
		log_later(_msgs[t_id]);
		_msgs.erase(t_id);
		break;
	case szlog::str_mod::flush:
		log_now(_msgs[t_id]);
		_msgs.erase(t_id);
		break;
	case szlog::str_mod::block:
		break;
	}

	return *this;
}


priority PriorityForLevel(int level) {
	if (level == 0) {
		return szlog::critical;
	} else if (level <= 2) {
		return szlog::error;
	} else if (level <= 5) {
		return szlog::warning;
	} else if (level <= 7) {
		return szlog::info;
	} else {
		return szlog::debug;
	}
}


// To substitute with std::put_time in (g++ >= 5)
std::string format_date(tm* localtime_t) {
	std::stringstream oss;
	std::vector<std::pair<int, char>> date_fields_with_postfixes = {{localtime_t->tm_year + 1900, '-'}, {localtime_t->tm_mon + 1, '-'}, {localtime_t->tm_mday, ' '}, {localtime_t->tm_hour, ':'}, {localtime_t->tm_min, ':'}, {localtime_t->tm_sec, ' '}};
	for (auto c: date_fields_with_postfixes) {

		// If less than 10 we need to add the "0" in front (00:02 instead of 0:2)
		if (c.first < 10) {
			oss << "0";
		}

		oss << c.first << c.second;
	}

	return oss.str();
}

const std::string msg_priority_for_level(szlog::priority p) {
	switch (p) {
	case szlog::debug:
		return "DEBUG";
	case szlog::info:
		return "INFO";
	case szlog::warning:
		return "WARN";
	case szlog::error:
		return "ERROR";
	case szlog::critical:
		return "CRITICAL";
	}

	return "";
}

Logger& log() {
	if (!szlog::logger)
		szlog::logger = std::make_shared<Logger>();

	return *szlog::logger;
}


} // namespace szlog

void sz_log_info(int info) {}
