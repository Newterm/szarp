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
int ipc_value(lua_State *lua);
int sz4base_value(lua_State *lua);
int pushTimeNow(lua_State* lua);

int main(int argc, char ** argv)
{
	Defdmn& dmn = Defdmn::getObject();
	dmn.configure(&argc, argv);

	dmn.go();
}

void Defdmn::MainLoop() {
	executeScripts();
	
	m_zmq->publish();
	if (m_cfg->GetDevice()->isParcookDevice()) m_ipc->GoParcook();
}

void Defdmn::go() {
	Defdmn::cycle_timer_callback(-1, 0, this);
	event_base_dispatch(m_event_base);
}

void Defdmn::executeScripts() {
	for (const auto& i: param_info) {
		try {
			i->executeAndUpdate(*m_zmq, m_read);
		} catch (SzException &e) {
			sz_log(0, "%s", e.what());
			throw;
		}
	}
}

double Defdmn::getParamData(TParam* p, sz4::nanosecond_time_t t, SZARP_PROBE_TYPE pt) {
	double result;
	sz4::weighted_sum<double, sz4::nanosecond_time_t> sum;

	m_base->get_weighted_sum(p, sz4::time_just_before<sz4::nanosecond_time_t>::get(t), t, pt, sum); // poprawna dana

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
		if (m_ipc->Init())
			throw SzException("Could not initialize IPC");
		sz_log(2, "IPC initialized successfully");
	} else {
		sz_log(2, "Single mode, ipc not intialized!!!");
	}

	TDevice * dev = m_cfg->GetDevice();

	m_read = m_ipc->m_read;
	m_cycle = dev->getDeviceTimeval();
	m_conf_str = m_cfg->GetPrintableDeviceXMLString();

	for (TUnit* unit = dev->GetFirstRadio()->GetFirstUnit();
			unit; unit = unit->GetNext()) {

		m_read_count += unit->GetParamsCount();

		for (TParam * p = unit->GetFirstParam(); p; p = unit->GetNextParam(p)) {
			try {
				std::wstring formula = p->GetParcookFormula(true);
				if (p->GetLuaScript()) {
						DefParamBase *pi = sz4::factory<DefParamBase, LuaParamBuilder>::op(p, p, p->GetIpcInd(), m_lua);
						pi->prepareParamsFromScript();
						param_info.push_back(pi);
				} else if (!formula.empty()) {
					std::wstring::size_type idx = formula.rfind(L'#');
					if (idx != std::wstring::npos)
						formula = formula.substr(0, idx);
					DefParamBase* pi = sz4::factory<DefParamBase, RPNParamBuilder>::op(p, p, p->GetIpcInd(), formula);
					pi->prepareParamsFromScript();
					param_info.push_back(pi);
				}
			} catch (SzException& e) {
				// if any param is ill-formed, stop the daemon
				sz_log(0, "%s", e.what());
				throw;
			}
		}
	}

	{ // Manage zmq and sz4 base
		char* sub_address = libpar_getpar("parhub", "pub_conn_addr", 1); // used for cache
		char* pub_address = libpar_getpar("parhub", "sub_conn_addr", 1); // we publish on parhub's subscribe address

		sz4::live_cache_config* live_config = new sz4::live_cache_config();
		live_config->retention = 1000;
		
		//live_config->urls.push_back(std::make_pair(sub_address, IPKContainer::GetObject()->GetConfig(SC::A2S(libpar_getpar("", "prefix", 1)))));

		configureHubs(live_config);

		Defdmn::m_base.reset(new sz4::base(L"/opt/szarp", IPKContainer::GetObject(), live_config));

		zmq::context_t* zmq_ctx = new zmq::context_t(1);
		m_zmq.reset(new zmqhandler(m_cfg->GetIPK(), m_cfg->GetDevice(), *zmq_ctx, sub_address, pub_address));
		sz_log(2, "ZMQ initialized successfully");
	} // throws 

	std::for_each(param_info.begin(), param_info.end(), [this](DefParamBase*& p){ p->subscribe_on_params(m_base.get()); });
}


Defdmn::Defdmn(): m_ipc(nullptr), m_cfg(nullptr), m_zmq(nullptr), m_base(nullptr), m_conf_str("/opt/szarp"), m_read(nullptr), m_read_count(0), m_event_base(nullptr), m_lua(lua_open()), param_info(), m_params() {
	// Prepare lua
	luaL_openlibs(m_lua);
	lua::set_probe_types_globals(m_lua);

	lua_register(m_lua, "param_value", ipc_value);
	lua_register(m_lua, "szbase", sz4base_value);
	lua_register(m_lua, "szb_move_time", sz4::lua_sz4_move_time);
	lua_register(m_lua, "szb_round_time", sz4::lua_sz4_round_time);
	lua_register(m_lua, "time_now", pushTimeNow);
	lua_register(m_lua, "isnan", sz4::lua_sz4_isnan);
	lua_register(m_lua, "nan", sz4::lua_sz4_nan);
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

int pushTimeNow(lua_State* lua) {
	lua_pushnumber(lua, (double) sz4::getTimeNow<sz4::nanosecond_time_t>());
	return 1;
}

int ipc_value(lua_State *lua) {
	const char* str = luaL_checkstring(lua, 1);

	double result = Defdmn::IPCParamValue(SC::U2S((const unsigned char*)str));

	lua_pushnumber(lua, result);

	return 1;
}

int sz4base_value(lua_State *lua) {
	const unsigned char* param_name = (unsigned char*) luaL_checkstring(lua, 1);
	if (param_name == NULL) 
		luaL_error(lua, "Invalid param name");

	auto time = sz4::make_nanosecond_time(lua_tonumber(lua, 2), 0);
	SZARP_PROBE_TYPE probe_type(static_cast<SZARP_PROBE_TYPE>((int)lua_tonumber(lua, 3)));

	double result = Defdmn::Sz4BaseValue(SC::U2S(param_name), time, probe_type);

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
