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
#ifndef SZB_PARAM_MONITOR_H__
#define SZB_PARAM_MONITOR_H__

#include "config.h"

#include <tr1/tuple>
#include <tr1/unordered_map>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>

#ifdef MINGW32
#include <winsock2.h>
#include <windows.h>
#include <unordered_map>
#endif

typedef int SzbMonitorTokenType;

class SzbParamMonitor;

class SzbParamMonitorImplBase {
protected:
	bool m_terminate = false;

	boost::thread m_thread;
	boost::mutex m_mutex;

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
		unsigned order;
	};

	std::list<command> m_queue;

	virtual bool trigger_command_pipe() = 0;
public:
	bool start_monitoring_dir(const std::string& path, TParam* param, SzbParamObserver* observer, unsigned order);
	bool end_monitoring_dir(SzbMonitorTokenType token);

	bool terminate();
};

#ifndef MINGW32
class SzbParamMonitorImpl : public SzbParamMonitorImplBase {

	SzbParamMonitor* m_monitor;

	int m_cmd_socket[2];
	int m_inotify_socket;

	void process_notification();
	void process_cmds();
	void loop();
	bool trigger_command_pipe() override;
public:
	SzbParamMonitorImpl(SzbParamMonitor *monitork, const std::string& szarp_data_dir);
	void run();
};

#else
class SzbParamMonitorImpl : public SzbParamMonitorImplBase {
	SzbMonitorTokenType m_token;

	std::string m_szarp_data_dir;		

	SzbParamMonitor* m_monitor;
	HANDLE m_dir_handle;

	OVERLAPPED m_overlapped;
	HANDLE m_event[2];

	DWORD m_buffer[65535];

	std::unordered_map<std::wstring, SzbMonitorTokenType> m_registry;

	bool m_teminate = false;

	void wait_for_changes();
	void loop();

	void process_file(PFILE_NOTIFY_INFORMATION info, std::vector<std::pair<SzbMonitorTokenType, std::string>>& tokens_and_paths);
	void process_notification();

	void process_cmds();

	void add_dir(const std::string& path, TParam* param, SzbParamObserver* observer, unsigned order);
	void del_dir(SzbMonitorTokenType token);

	bool trigger_command_pipe() override;
public:
	SzbParamMonitorImpl(SzbParamMonitor *monitor, const std::string& szarp_data_dir);

	void run();

	~SzbParamMonitorImpl();
};

#endif

class SzbParamMonitor {
	friend class SzbParamMonitorImpl;

	std::tr1::unordered_map<SzbMonitorTokenType, std::multimap<unsigned, std::pair<SzbParamObserver*, TParam*> > > m_token_observer_param;
	std::tr1::unordered_map<SzbParamObserver*, std::vector<std::tr1::tuple<SzbMonitorTokenType, TParam*, std::string> > > m_observer_token_param;
	std::tr1::unordered_map<std::string, SzbMonitorTokenType> m_path_token;
	std::tr1::unordered_map<SzbMonitorTokenType, std::string> m_token_path;

	boost::mutex m_mutex;
	boost::condition m_cond;
	
	SzbParamMonitorImpl m_monitor_impl;

	void modify_dir_token(SzbMonitorTokenType old_wd, SzbMonitorTokenType new_wd);
public:
	SzbParamMonitor(const std::wstring& szarp_data_dir);
	void dir_registered(SzbMonitorTokenType token, TParam *param, SzbParamObserver* observer, const std::string& path, unsigned order);
	void failed_to_register_dir(TParam *param, SzbParamObserver* observer, const std::string& path);
	void files_changed(const std::vector<std::pair<SzbMonitorTokenType, std::string> >& tokens_and_paths);
	void terminate();
	void add_observer(SzbParamObserver* observer, TParam* param, const std::string& path, unsigned order);
	void add_observer(SzbParamObserver* observer, const std::vector<std::pair<TParam*, std::vector<std::string> > >& observed, unsigned order);
	void remove_observer(SzbParamObserver* obs);
	~SzbParamMonitor();	
};



#endif
