#ifndef __DEFDMN__H__
#define __DEFDMN__H__

#ifndef NO_LUA

#include <zmq.hpp>
#include <string>
#include <vector>
#include <event.h>
#include <evdns.h>
#include <stdexcept>
#include "szarp.h"
#include <lua.hpp>
#include "szarp_config.h"
#include "zmqhandler.h"
#include "ipchandler.h"
#include "ipcdefines.h"

#include <tr1/unordered_map>

#include "daemon.h"
#include "sz4/defs.h"

#include <boost/thread/recursive_mutex.hpp>

#include "szarp_base_common/szbparamobserver.h"

#include "sz4/param_observer.h"
#include "szarp_base_common/szbparammonitor.h"
#include "sz4/param_entry.h"

#include "sz4/block.h"
#include "sz4/buffer.h"
#include "sz4/factory.h"
#include "sz4/live_cache.h"
#include "sz4/block_cache.h"
#include "sz4/live_observer.h"
#include "sz4/real_param_entry.h"
#include "sz4/real_param_entry_templ.h"
#include "sz4/base.h"

#include "conversion.h"

#include <iostream>

#include "sz4/util.h"
#include "sz4/time.h"
#include <ctime>
#include <unordered_map>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

class DefParamBase;

class Defdmn {
	Defdmn();
public:
	static Defdmn& getObject() {
		static Defdmn instance;
		return instance;
	}

	void go();

	void configure(int* argc, char** argv);

	static void cycle_timer_callback(int fd, short event, void* arg);

	virtual ~Defdmn() {}

protected:
	void configure_events();

	void MainLoop(); 

	double executeScript(DefParamBase*);
	void executeScripts();
	void Calcul(DefParamBase*);

private:
	std::unique_ptr<IPCHandler> m_ipc;
	std::unique_ptr<DaemonConfig> m_cfg;
	std::unique_ptr<zmqhandler> m_zmq;
	std::unique_ptr<sz4::base> m_base;

	std::string m_conf_str;

	short* m_read;
	size_t m_read_count;

	struct event_base* m_event_base;

	lua_State *m_lua;
	std::vector<DefParamBase*> param_info;

	struct event m_timer;
	struct timeval m_cycle;

public:
	double getParamData(TParam* p, sz4::nanosecond_time_t t, SZARP_PROBE_TYPE pt = SZARP_PROBE_TYPE::PT_HALFSEC);

private:
	std::mutex m_mutex;

	std::map<std::wstring, std::vector<TParam*>> m_params;

public:
	static double IPCParamValue(const std::wstring& name) {
		TParam* param = IPKContainer::GetObject()->GetParam(name);
		if (!param) return std::numeric_limits<double>::min();

		sz4::nanosecond_time_t data_time;
		Defdmn::getObject().m_base->get_last_time(param, data_time);

		return Defdmn::getObject().getParamData(param, data_time);
	}

	static double Sz4BaseValue(const std::wstring& name, sz4::nanosecond_time_t t, SZARP_PROBE_TYPE pt) {
		TParam* param = IPKContainer::GetObject()->GetParam(name);
		if (!param) return std::numeric_limits<double>::min();

		return Defdmn::getObject().getParamData(param, t, pt);
	}
};

#include "defdmn.tpp"
#endif
#endif
