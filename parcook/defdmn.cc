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
/* $Id$ */

/*
 * SZARP 2.0
 * parcook
 * parcook.c
 */

#ifndef NO_LUA


#include "sz4/lua_interpreter_templ.h"
#include <sstream>
#include <assert.h>

#include "conversion.h"

#include "exception.h"
#include "liblog.h"
#include "libpar.h"

#include <regex>
#include <vector>

#include <iostream>
#include <algorithm>

#include <utility>
#include <boost/lexical_cast.hpp>
#include "defdmn.h"

void configureHubs(sz4::live_cache_config*);
void registerLuaFunctions();
int sz4base_value(lua_State *lua);
int ipc_value(lua_State *lua);

int main(int argc, char ** argv)
{
	Defdmn& dmn = Defdmn::getObject();
	dmn.configure(&argc, argv);

	dmn.go();
}

Defdmn::Defdmn(): m_ipc(nullptr), m_cfg(nullptr), m_zmq(nullptr), m_base(nullptr), m_event_base(nullptr), param_info() {}

void Defdmn::MainLoop() {
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
			i->executeAndUpdate(*m_zmq);
			if (connectToParcook) i->sendValueToParcook(m_ipc->m_read);
		} catch (SzException &e) {
			sz_log(0, "%s", e.what());
			throw;
		}
	}
}

double Defdmn::getParamData(TParam* p, sz4::nanosecond_time_t t, SZARP_PROBE_TYPE pt) {
	double result;
	sz4::weighted_sum<double, sz4::nanosecond_time_t> sum;

	m_base->get_weighted_sum(p, sz4::time_just_before<sz4::nanosecond_time_t>::get(t), t, pt, sum);

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
	IPKContainer::Init(L"/opt/szarp", L"/opt/szarp", L"pl");
	m_cfg.reset(new DaemonConfig("defdmn"));

	configure_events();
	
	if (m_cfg->Load(argc, argv, 0, NULL, 0, m_event_base))
		throw SzException("Could not load configuration");

	m_cfg->GetIPK()->SetConfigId(0);

	m_ipc.reset(new IPCHandler(m_cfg.get()));

	if (!m_cfg->GetSingle()) {
		if (m_ipc->Init()) {
			sz_log(0, "Could not initialize IPC");
		} else {
			connectToParcook = true;
			sz_log(2, "IPC initialized successfully");
		}
	} else {
		sz_log(2, "Single mode, ipc not intialized!!!");
	}

	TDevice * dev = m_cfg->GetDevice();
	dev->configureDeviceTimeval(boost::lexical_cast<long int>(libpar_getpar("sz4", "heartbeat_frequency", 1)));
	m_cycle = dev->getDeviceTimeval();

	sz4::live_cache_config* live_config = new sz4::live_cache_config();
	live_config->retention = 1000;
	
	configureHubs(live_config);

	char* sub_address = libpar_getpar("parhub", "pub_conn_addr", 1);
	char* pub_address = libpar_getpar("parhub", "sub_conn_addr", 1);

	Defdmn::m_base.reset(new sz4::base(L"/opt/szarp", IPKContainer::GetObject(), live_config));
	m_zmq.reset(new zmqhandler(m_cfg->GetIPK(), dev, *new zmq::context_t(1), sub_address, pub_address)); // TODO: in single publish on another address
	sz_log(2, "ZMQ initialized successfully");

	registerLuaFunctions();

	for (TUnit* unit = dev->GetFirstRadio()->GetFirstUnit(); unit; unit = unit->GetNext()) {
		for (TParam * p = unit->GetFirstParam(); p; p = unit->GetNextParam(p)) {
			if (p->GetLuaScript()) {
				param_info.push_back(*new std::shared_ptr<DefParamBase>(sz4::factory<DefParamBase, LuaParamBuilder>::op(p, p, p->GetIpcInd())));
			} else {
				std::wstring formula = p->GetParcookFormula(true);
				if (!formula.empty()) {
					param_info.push_back(*new std::shared_ptr<DefParamBase>(sz4::factory<DefParamBase, RPNParamBuilder>::op(p, p, p->GetIpcInd(), formula)));
				} else {
					throw SzException("Param "+SC::S2A(p->GetGlobalName())+" was ill-formed");
				}
			}
			param_info.back()->prepareParamsFromScript();
		} // if any param is ill-formed, stop the daemon
	}

	for (auto& p: param_info) p->subscribe_on_params(m_base.get());
}

double Defdmn::IPCParamValue(const std::wstring& name) {
	TParam* param = IPKContainer::GetObject()->GetParam(name);
	if (!param) return std::numeric_limits<double>::min();

	sz4::nanosecond_time_t data_time;
	Defdmn::getObject().m_base->get_last_time(param, data_time);

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

float ChooseFun(float funid, float *parlst)
{
	ushort fid;

	fid = (ushort) funid;
	if (fid >= MAX_FID)
		return (0.0);
	return ((*(FunTable[fid])) (parlst));
}

void putParamsFromString(const std::wstring& script_string, std::wregex& ipc_par_reg, const int& name_match_prefix, const int& name_match_sufix, std::vector<std::wstring>& ret_params) {
	std::wsmatch pmatch;
	for (auto searchStart = script_string.cbegin(); std::regex_search(searchStart, script_string.cend(), pmatch, ipc_par_reg); searchStart += pmatch.position() + pmatch.length()) {
		std::wstring name(pmatch.str().substr(name_match_prefix, pmatch.length()-name_match_sufix));
		ret_params.push_back(name);
	}
}

std::wstring makeParamNameGlobal(const char* pname) {
	std::wstring name(SC::U2S( (const unsigned char*) pname));
	if (std::count(name.begin(), name.end(), L':') == 2) name.insert(0, SC::A2S(libpar_getpar("", "prefix", 0)) + (wchar_t)':');
	return name;
}

int ipc_value(lua_State *lua) {
	const char* str = luaL_checkstring(lua, 1);

	double result = Defdmn::IPCParamValue(makeParamNameGlobal(str));

	lua_pushnumber(lua, result);

	return 1;
}

int sz4base_value(lua_State *lua) {
	const char* param_name = luaL_checkstring(lua, 1);
	if (param_name == NULL) 
		luaL_error(lua, "Invalid param name");

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
			live_config->urls.push_back(std::make_pair(address, IPKContainer::GetObject()->GetConfig(prefix)));
		}
		std::free(_prefix);
		std::free(_address);
	}
	std::free(_servers);
}

#endif
