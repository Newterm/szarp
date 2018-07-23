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

/*
 * Demon do wyliczania parametrów ze skryptów LUA i formu³ RPN
 * 
 * Mateusz Morusiewicz <mmorusiewicz@newterm.pl>
 * 
 * $Id$
 */

/*
 SZARP daemon description block.

 @description_start

 @class 4

 @devices Daemon processes LUA and RPN formulas
 @devices.pl Sterownik wylicza parametry ze skryptów LUA i formu³ RPN

 @config Daemon is configured in szarp.cfg.

 @config.pl Sterownik jest konfigurowany w pliku szarp.cfg.

 @config_example
 <device 
      xmlns:exec="http://www.praterm.com.pl/SZARP/ipk-extra"
      daemon="/opt/szarp/bin/defdmn" 
      data-timeout="0" [default is 1h, 0 disables (never expire)]
      options="--some-option -f some-argument">
      ...

 # in szarp.cfg
 :available parhubs
 servers=local_hub some_other

 :local_hub
 prefix=$prefix$
 address=tcp://127.0.0.1:56662

 :some_other
 prefix=othr
 address=tcp://192.168.0.42:56662

 @description_end
*/


#ifndef NO_LUA


#include "sz4/lua_interpreter_templ.h"
#include "ipkcontainer.h"
#include <sstream>
#include <assert.h>

#include "conversion.h"

#include "exception.h"
#include "liblog.h"
#include "libpar.h"

#include <boost/regex.hpp>
#include <vector>

#include <iostream>
#include <algorithm>

#include <utility>
#include <boost/lexical_cast.hpp>
#include "defdmn.h"

sz4::nanosecond_time_t Defdmn::time_now = sz4::time_trait<sz4::nanosecond_time_t>::invalid_value;
sz4::second_time_t Defdmn::data_timeout = sz4::time_trait<sz4::second_time_t>::invalid_value;

void configureHubs(sz4::live_cache_config*);
void registerLuaFunctions();
int sz4base_value(lua_State *lua);
int ipc_value(lua_State *lua);

int main(int argc, char ** argv)
{
	try {
		Defdmn& dmn = Defdmn::getObject();
		dmn.configure(&argc, argv);

		dmn.go();
	} catch (const std::exception& e) {
		sz_log(0, "Error: %s, exiting.", e.what());
	}
}

Defdmn::Defdmn(): m_ipc(nullptr), m_cfg(nullptr), m_zmq(nullptr), m_base(nullptr), m_event_base(nullptr), param_info() {}

void Defdmn::MainLoop() {
	// update time
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
	Defdmn::time_now = sz4::nanosecond_time_t(seconds, nanoseconds);

	executeScripts();
	
	if (!m_cfg->GetSingle()) {
		m_zmq->publish();
		if (connectToParcook) m_ipc->GoParcook();
	}
}

void Defdmn::go() {
	Defdmn::cycle_timer_callback(-1, 0, this);
	event_base_dispatch(m_event_base);
}

void Defdmn::executeScripts() {
	for (auto& i: param_info) {
		try {
			i->execute();
			if (!single) i->update(*m_zmq);
			if (!single && connectToParcook) i->sendValueToParcook(m_ipc->m_read);
		} catch (SzException &e) {
			sz_log(1, "Error while calculating param: %s", e.what());

			// set nodata
			i->setVal(sz4::no_data<double>());
			if (!single) i->update(*m_zmq);
		}
	}
}

double Defdmn::getParamData(TParam* p, sz4::nanosecond_time_t t, SZARP_PROBE_TYPE pt) {
	double result;
	sz4::weighted_sum<double, sz4::nanosecond_time_t> sum;

	m_base->get_weighted_sum(p, sz4::time_just_before(t), t, pt, sum);

	result = sz4::scale_value(sum.avg(), p);
	return result;
}

void Defdmn::configure_events() {
	m_event_base = (struct event_base*) event_init();
	if (!m_event_base) {
		throw SzException("Could not initialize event base");
	}

	evtimer_set(&m_timer, Defdmn::cycle_timer_callback, this);
	event_base_set(m_event_base, &m_timer);
}

void Defdmn::cycle_timer_callback(int fd, short event, void* arg) {
	Defdmn* def_ptr = (Defdmn*) arg;

	def_ptr->MainLoop();

	evtimer_add(&def_ptr->m_timer, &def_ptr->m_cycle);
}

void Defdmn::configure(int* argc, char** argv) {
	ParamCachingIPKContainer::Init(L"/opt/szarp", L"/opt/szarp", L"pl");
	m_cfg.reset(new DaemonConfig("defdmn"));

	configure_events();

	if (m_cfg->Load(argc, argv))
		throw SzException("Could not load configuration");

	m_cfg->GetIPK()->SetConfigId(0);
	single = m_cfg->GetSingle();

	try {
		m_ipc.reset(new IPCHandler(m_cfg.get()));
		sz_log(2, "IPC initialized successfully");
		connectToParcook = true;
	} catch(...) {
		sz_log(1, "Could not initialize IPC");
	}

	TDevice * dev = m_cfg->GetDevice();
	Defdmn::data_timeout = dev->getAttribute("data-timeout", 3600); // default is 1 hour
	m_cycle = m_cfg->GetDeviceTimeval();

	sz4::live_cache_config* live_config = new sz4::live_cache_config();
	live_config->retention = 1000;

	configureHubs(live_config);

	char* sub_address = libpar_getpar("parhub", "pub_conn_addr", 1);
	char* pub_address = libpar_getpar("parhub", "sub_conn_addr", 1);

	Defdmn::m_base.reset(new sz4::base(L"/opt/szarp", IPKContainer::GetObject(), live_config));
	m_zmq.reset(new zmqhandler(m_cfg.get(), context, sub_address, pub_address)); // TODO: in single publish on another address
	sz_log(2, "ZMQ initialized successfully");

	registerLuaFunctions();

	size_t i = 0;
	for (TUnit* unit = dev->GetFirstUnit(); unit; unit = unit->GetNext()) {
		for (TParam * p = unit->GetFirstParam(); p; p = p->GetNext()) {
			if (p->GetLuaScript()) {
				std::shared_ptr<DefParamBase> ptr(sz4::factory<DefParamBase, LuaParamBuilder>::op(p, p, i++));
				param_info.push_back(ptr);
			} else {
				std::shared_ptr<DefParamBase> ptr(sz4::factory<DefParamBase, RPNParamBuilder>::op(p, p, i++));
				param_info.push_back(ptr);
			}
		} // if any param is ill-formed, stop the daemon
	}

	for (auto& p: param_info) p->subscribe_on_params(m_base.get());
}

std::wstring makeParamNameGlobal(std::wstring name) {
	if (std::count(name.begin(), name.end(), L':') == 2) {
		std::wstring prefix = SC::A2S(libpar_getpar("", "prefix", 0));
		prefix.erase(std::remove(prefix.begin(), prefix.end(), (wchar_t)'/'), prefix.end());

		name.insert(0, prefix + (wchar_t)':');
	}
	return name;
}

std::wstring makeParamNameGlobal(const char* pname) {
	std::wstring name(SC::U2S( (const unsigned char*) pname));
	return makeParamNameGlobal(std::move(name));
}

boost::optional<double> Defdmn::IPCParamValue(const std::wstring& name) {
	std::wstring global_name = makeParamNameGlobal(name);

	return getGlobalParamValue(global_name);
}

boost::optional<double> Defdmn::IPCParamValue(const char* pname) {
	std::wstring global_name = makeParamNameGlobal(pname);

	return getGlobalParamValue(global_name);
}

boost::optional<double> Defdmn::getGlobalParamValue(const std::wstring& name) {
	TParam* param = IPKContainer::GetObject()->GetParam(name);
	if (!param)
		return boost::none;

	sz4::nanosecond_time_t data_time;
	Defdmn::getObject().m_base->get_last_time(param, data_time);

	auto valid_till = data_time.second + Defdmn::data_timeout;
	if (Defdmn::data_timeout != 0 && valid_till < Defdmn::time_now.second)
		return boost::none;

	return Defdmn::getObject().getParamData(param, data_time);
}

double Defdmn::Sz4BaseValue(const std::wstring& name, sz4::nanosecond_time_t t, SZARP_PROBE_TYPE pt) {
	TParam* param = IPKContainer::GetObject()->GetParam(name);
	if (!param) return std::numeric_limits<double>::min();

	return Defdmn::getObject().getParamData(param, t, pt);
}

void registerLuaFunctions() {
	lua_State* lua = Defdmn::getObject().get_lua_interpreter().lua();
	lua_register(lua, "param_value", ipc_value);
	lua_register(lua, "szbase", sz4base_value);
}

int ipc_value(lua_State *lua) {
	const char* str = luaL_checkstring(lua, 1);
	if (str == NULL)
		return luaL_error(lua, "Invalid param name");

	auto result = Defdmn::IPCParamValue(str);
	if (!result)
		lua_pushnil(lua);
	else
		lua_pushnumber(lua, *result);

	return 1;
}

int sz4base_value(lua_State *lua) {
	const char* param_name = luaL_checkstring(lua, 1);
	if (param_name == NULL) 
		return luaL_error(lua, "Invalid param name");

	auto time = sz4::make_nanosecond_time((double) lua_tonumber(lua, 2), 0);
	SZARP_PROBE_TYPE probe_type(static_cast<SZARP_PROBE_TYPE>((int) lua_tonumber(lua, 3)));

	double result = Defdmn::Sz4BaseValue(makeParamNameGlobal(param_name), time, probe_type);

	lua_pushnumber(lua, result);
	return 1;
}

void configureHubs(sz4::live_cache_config* live_config) {
	char *_servers = libpar_getpar("available_parhubs", "servers", 0);
	if (_servers == NULL)
		return;

	std::stringstream ss(_servers);
	std::string section;
	while (ss >> section) {
		char *_prefix = libpar_getpar(section.c_str(), "prefix", 0);
		char *_address = libpar_getpar(section.c_str(), "address", 0);
		if (_address == NULL) _address = libpar_getpar("parhub", "pub_conn_addr", 1);
		if (_prefix != NULL) {
			std::string address(_address);
			std::wstring prefix = SC::A2S(_prefix);
			auto ipk = IPKContainer::GetObject()->GetConfig(prefix);
			if (ipk) {
				live_config->urls.push_back(std::make_pair(address, ipk));
			} else {
				sz_log(1, "Error while configuring IPK for base %ls.", prefix.c_str());
			}
		}
		std::free(_prefix);
		std::free(_address);
	}
	std::free(_servers);
}

#endif
