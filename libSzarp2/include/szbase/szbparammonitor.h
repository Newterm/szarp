#ifndef SZB_PARAM_MONITOR_H__
#define SZB_PARAM_MONITOR_H__
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

#include <tr1/tuple>
#include <tr1/unordered_map>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>

typedef int SzbMonitorTokenType;

class SzbParamMonitor;
class SzbParamMonitorImpl {
	boost::thread m_thread;
	boost::mutex m_mutex;
	bool m_terminate;

	SzbParamMonitor* m_monitor;

	enum CMD {
		ADD_CMD,
		DEL_CMD,
		END_CMD
	};

	struct command {
		CMD cmd;		
		std::string path;
		SzbMonitorTokenType token;
		TParam* param;
		SzbParamObserver* observer;
	};

	int m_cmd_socket[2];
	int m_inotify_socket;
	std::list<command> m_queue;

	void process_notification();
	void process_cmds();
	void loop();
	bool trigger_command_pipe();
public:
	SzbParamMonitorImpl(SzbParamMonitor *monitork);
	bool start_monitoring_dir(const std::string& path, TParam* param, SzbParamObserver* observer);
	bool end_monitoring_dir(SzbMonitorTokenType token);
	void run();
	bool terminate();
};

class SzbParamMonitor {
	friend class SzbParamMonitorImpl;

	std::tr1::unordered_map<SzbMonitorTokenType, std::vector<std::pair<SzbParamObserver*, TParam*> > > m_token_observer_param;
	std::tr1::unordered_map<SzbParamObserver*, std::vector<std::tr1::tuple<SzbMonitorTokenType, TParam*, std::string> > > m_observer_token_param;
	std::tr1::unordered_map<std::string, SzbMonitorTokenType> m_path_token;

	boost::mutex m_mutex;
	boost::condition m_cond;
	
	SzbParamMonitorImpl m_monitor_impl;
public:
	SzbParamMonitor();	
	void dir_registered(SzbMonitorTokenType token, TParam *param, SzbParamObserver* observer, const std::string& path);
	void failed_to_register_dir(TParam *param, SzbParamObserver* observer, const std::string& path);
	void dirs_changed(const std::vector<SzbMonitorTokenType>& dirs);
	void terminate();
	void add_observer(SzbParamObserver* observer, const std::vector<std::pair<TParam*, std::vector<std::string> > >& observed);
	void remove_observer(SzbParamObserver* obs);
	~SzbParamMonitor();	
};



#endif
