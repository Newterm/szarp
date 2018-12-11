/* 
  SZARP: SCADA software 

*/
/*
 * szbase - szbbase.c - main public interfaces
 * $Id$
 * <pawel@praterm.com.pl>
 */

#include <cassert>
#include <algorithm>
#include <sstream>
#include <vector>
#include <ctime>

#ifdef MINGW32
#undef GetObject
#endif

#include <liblog.h>
#include "szarp_config.h"
#include "szbbase.h"
#include "szarp_base_common/szbparamobserver.h"
#include "szarp_base_common/szbparammonitor.h"
#include "szbbuf.h"
#include "szbhash.h"

#include "cacheabledatablock.h"
#include "conversion.h"
#include "proberconnection.h"
#include "szarp_base_common/lua_utils.h"
#include "integrator.h"

#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
#include "loptcalculate.h"
#endif
#ifndef luaL_reg
#define luaL_reg luaL_Reg
#endif
#ifndef lua_open
#define lua_open  luaL_newstate
#endif
#endif

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

#ifdef MINGW32
#undef GetObject
#endif

using std::find;
using std::string;
using std::pair;
using std::tr1::unordered_map;

const int default_szbuffer_cache_size = 12 * 12 * 5 * 2;	/** 12 graphs * 12 months * 5 block for param and make it double*/

const int default_maximum_search_time = 15;			/** maximum deafult serach time */

Szbase::Szbase(const std::wstring& szarp_dir) : m_szarp_dir(szarp_dir), m_config_modification_callback(NULL)
{
	m_monitor = new SzbParamMonitor(szarp_dir);
	m_current_query = 0;
	m_maximum_search_time = default_maximum_search_time;
	m_ipk_containter = IPKContainer::GetObject();
}

int Szbase::GetMaximumSearchTime() const {
	return m_maximum_search_time;
}

void Szbase::SetMaximumSerachTime(int serach_time) {
	m_maximum_search_time = serach_time;
}

void Szbase::Destroy() {
	delete _instance;
	_instance = NULL;
}

Szbase::~Szbase() {
	delete m_monitor;
	m_file_watcher.Terminate();
	for (std::vector<szb_buffer_t*>::iterator i = m_szb_buffers.begin(); i != m_szb_buffers.end(); i++)
		if (*i)
			szb_free_buffer(*i);
}

void Szbase::Init(const std::wstring& szarp_dir, bool write_cache) {
	assert(_instance == NULL);

	CacheableDatablock::write_cache = write_cache;

	_instance = new Szbase(szarp_dir);
	_instance->m_buffer_cache_size = default_szbuffer_cache_size;

}

void Szbase::Init(const std::wstring& szarp_dir, void (*callback)(std::wstring, std::wstring), bool write_cache, int memory_cache_size) {
	assert(_instance == NULL);

	CacheableDatablock::write_cache = write_cache;

	_instance = new Szbase(szarp_dir);
	_instance->m_config_modification_callback = callback;

	if (memory_cache_size)
		_instance->m_buffer_cache_size = memory_cache_size;
	else 
		_instance->m_buffer_cache_size = default_szbuffer_cache_size;
}

bool Szbase::AddBase(const std::wstring& prefix) {

#if BOOST_FILESYSTEM_VERSION == 3
	return AddBase((m_szarp_dir / prefix / L"szbase").wstring(), prefix);
#else
	return AddBase((m_szarp_dir / prefix / L"szbase").file_string(), prefix);
#endif
}

void Szbase::AddExtraParam(const std::wstring &prefix, TParam *param) {
}

#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
void Szbase::AddLuaOptParamReference(TParam* refered, TParam* referring) {
	m_lua_opt_param_reference_map[refered].insert(referring);
}
#endif
#endif

void Szbase::RemoveExtraParam(const std::wstring& prefix, TParam *param) {
	szb_buffer_t* buffer = GetBuffer(prefix, false);
	if (buffer == NULL)
		return;

#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
	std::set<TParam*>& set = m_lua_opt_param_reference_map[param];
	for (std::set<TParam*>::iterator i = set.begin();
			i != set.end();
			i++) {
		TParam* p = *i;
		const std::wstring& prefix = p->GetSzarpConfig()->GetPrefix();

		szb_buffer_t* ref_buffer = GetBuffer(prefix, false);
		if (ref_buffer)
			ref_buffer->RemoveExecParam(p);	
		ClearParamFromCache(prefix, p);
		p->SetLuaExecParam(NULL);
	}
	m_lua_opt_param_reference_map.erase(param);
	LuaExec::Param *ep = param->GetLuaExecParam();
	if (ep) {
		for (std::vector<LuaExec::ParamRef>::iterator i = ep->m_par_refs.begin();
			 	i != ep->m_par_refs.end();
				i++) {
			m_lua_opt_param_reference_map[i->m_param].erase(param);
			if (m_lua_opt_param_reference_map[i->m_param].empty())
				m_lua_opt_param_reference_map.erase(i->m_param);
		}
	}
#endif
#endif

	ClearParamFromCache(prefix, param);

	buffer->RemoveExecParam(param);
}

bool Szbase::AddBase(const std::wstring& szbase_dir, const std::wstring &prefix) {

	TSzarpConfig *ipk = m_ipk_containter->GetConfig(prefix);
	if (ipk == NULL) {
		sz_log(1, "Unable to load ipk dir:%ls", prefix.c_str());
		return false;
	}
		
	szb_buffer_t* szbbuf = szb_create_buffer(this, szbase_dir, m_buffer_cache_size, ipk);
	if (szbbuf == NULL) {
		sz_log(1, "Unable to load ipk dir:%ls", szbase_dir.c_str());
		return false;
	}

	if (m_szb_buffers.size() <= ipk->GetConfigId())
		m_szb_buffers.resize(ipk->GetConfigId() + 1, NULL);

	assert(m_szb_buffers[ipk->GetConfigId()] == NULL);
	m_szb_buffers[ipk->GetConfigId()] = szbbuf;

	if (m_config_modification_callback != NULL) {
		m_file_watcher.AddFileWatch(szbbuf->GetConfigurationFilePath(), prefix, m_config_modification_callback);
		m_file_watcher.AddFileWatch(szbbuf->GetSzbaseStampFilePath(), prefix, m_config_modification_callback);
	}

	return true;
}

void Szbase::NotifyAboutConfigurationChanges() {
	if (m_config_modification_callback != NULL) {
		m_file_watcher.CheckModifications();
	}
}

void Szbase::RemoveConfig(const std::wstring &prefix, bool poison_cache) {
	TSzarpConfig *ipk = m_ipk_containter->GetConfig(prefix);
	if (ipk == NULL)
		return;

	if (m_szb_buffers.size() <= ipk->GetConfigId())
		return;

	szb_buffer_t*& buffer = m_szb_buffers[ipk->GetConfigId()];
	if (buffer == NULL)
		return;

	m_file_watcher.RemoveFileWatch(prefix);

	if (poison_cache)
		buffer->cachepoison = true;
	szb_free_buffer(buffer);

	buffer = NULL;
}

std::wstring Szbase::GetCacheDir(const std::wstring &prefix) {
	return CacheableDatablock::GetCacheRootDirPath(Szbase::GetBuffer(prefix));
}

void Szbase::ClearCacheDir(const std::wstring& prefix) {
	Szbase::GetBuffer(prefix)->ClearCache();
}

void Szbase::ClearParamFromCache(const std::wstring& prefix, TParam* param) {
	Szbase::GetBuffer(prefix)->ClearParamFromCache(param);
}

bool Szbase::FindParam(const std::basic_string<unsigned char>& param, std::pair<szb_buffer_t*, TParam*>& bp) {
	bp.second = m_ipk_containter->GetParam(param);
	if (bp.second == NULL)
		return false;
	
	bp.first = GetBufferForParam(bp.second);
	if (bp.first == NULL)
		return false;

	return true;
}

bool Szbase::FindParam(const std::wstring& param, std::pair<szb_buffer_t*, TParam*>& bp) {
	bp.second = m_ipk_containter->GetParam(param);
	if (bp.second == NULL)
		return false;
	
	bp.first = GetBufferForParam(bp.second);
	if (bp.first == NULL)
		return false;

	return true;
}

time_t Szbase::SearchFirst(const std::wstring &param, bool &ok) {

	std::pair<szb_buffer_t*, TParam*> bp;
	ok = FindParam(param, bp);
	if (!ok)
		return -1;

	return szb_search_first(bp.first, bp.second);

}

time_t Szbase::SearchFirst(const std::basic_string<unsigned char> &param, bool &ok) {

	std::pair<szb_buffer_t*, TParam*> bp;
	ok = FindParam(param, bp);
	if (!ok)
		return -1;

	return szb_search_first(bp.first, bp.second);

}

time_t Szbase::SearchLast(const std::wstring& param, bool &ok) { 

	std::pair<szb_buffer_t*, TParam*> bp;

	ok = FindParam(param, bp);
	if (!ok)
		return -1;

	return szb_search_last(bp.first, bp.second);
}

time_t Szbase::SearchLast(const std::basic_string<unsigned char>& param, bool &ok) { 

	std::pair<szb_buffer_t*, TParam*> bp;

	ok = FindParam(param, bp);
	if (!ok)
		return -1;

	return szb_search_last(bp.first, bp.second);
}

time_t Szbase::Search(const std::wstring& param, time_t start, time_t end, int direction, SZARP_PROBE_TYPE probe, bool &ok, std::wstring& error) {

	std::pair<szb_buffer_t*, TParam*> bp;

	ok = FindParam(param, bp);
	if (!ok) {
		error = L"Param " + error + L" not found";
		return -1;
	}

	time_t t = szb_search(bp.first, bp.second, start, end, direction, probe);

	if (bp.first->last_err == SZBE_OK) 
		ok = true;
	else {
		ok = false;
		error = bp.first->last_err_string;
		bp.first->last_err = SZBE_OK;
	}

	return t;
}

double Szbase::GetValue(std::pair<szb_buffer_t*, TParam*>& bp, time_t time, SZARP_PROBE_TYPE probe_type, int custom_length, bool *is_fixed, bool &ok, std::wstring &error) { 	
	time_t end = szb_move_time(time, 1, probe_type, custom_length);
	double result = szb_get_avg(bp.first, bp.second, time, end, NULL, NULL, probe_type, is_fixed);

	if (bp.first->last_err == SZBE_OK) 
		ok = true;
	else {
		ok = false;
		error = bp.first->last_err_string;
		bp.first->last_err = SZBE_OK;
	}

	return result;

}

double Szbase::GetValue(const std::basic_string<unsigned char>& param, time_t time, SZARP_PROBE_TYPE probe_type, int custom_length, bool *is_fixed, bool &ok, std::wstring &error) {

	std::pair<szb_buffer_t*, TParam*> bp;
	ok = FindParam(param, bp);

	if (ok == false) {
		error = std::wstring(L"Param ") + SC::U2S(param) + L" not present";
		return SZB_NODATA;
	}

	return GetValue(bp, time, probe_type, custom_length, is_fixed, ok, error);

}

double Szbase::GetValue(const std::wstring& param, time_t time, SZARP_PROBE_TYPE probe_type, int custom_length, bool *is_fixed, bool &ok, std::wstring &error) {
	std::pair<szb_buffer_t*, TParam*> bp;
	ok = FindParam(param, bp);

	if (ok == false) {
		error = std::wstring(L"Param ") + param + L" not present";
		return SZB_NODATA;
	}

	return GetValue(bp, time, probe_type, custom_length, is_fixed, ok, error);

}


Szbase* Szbase::GetObject() {
	if (_instance == NULL)
		assert(false);

	return _instance;
}
Szbase* Szbase::_instance = NULL;

szb_buffer_t* Szbase::GetBufferForParam(TParam* p, bool add_if_not_present) {
	szb_buffer_t *buffer;
	if (m_szb_buffers.size() <= p->GetConfigId() || (buffer = m_szb_buffers[p->GetConfigId()]) == NULL) {
		if (add_if_not_present) {
			if (!AddBase(p->GetSzarpConfig()->GetPrefix()))
				return NULL;
			return GetBufferForParam(p, false);
		} else
			return NULL;
	}
	return buffer;
}

szb_buffer_t* Szbase::GetBuffer(const std::wstring &prefix, bool add_if_not_present) {
	TSzarpConfig *ipk = m_ipk_containter->GetConfig(prefix);
	if (ipk == NULL)
		return NULL;
	szb_buffer_t* buffer;
	if (m_szb_buffers.size() <= ipk->GetConfigId() || (buffer = m_szb_buffers[ipk->GetConfigId()]) == NULL) {
		if (add_if_not_present) {
			if (!AddBase(prefix))
				return NULL;
			return GetBuffer(prefix, false);
		} else
			return NULL;
	}
	return buffer;
}

void Szbase::NextQuery() {
	if (m_current_query == std::numeric_limits<long>::max())
		m_current_query = 0;

	m_current_query++;
}

void Szbase::SetProberAddress(std::wstring prefix, std::wstring address, std::wstring port) {
	m_probers_addresses[prefix] = std::make_pair(address, port);

	szb_buffer_t* buffer = GetBuffer(prefix);
	if (buffer) {
		delete buffer->prober_connection;
		buffer->prober_connection = NULL;
	}

}


bool Szbase::GetProberAddress(std::wstring prefix, std::wstring &address, std::wstring &port) {
	std::tr1::unordered_map<std::wstring, std::pair<std::wstring, std::wstring> >::iterator i;
	i = m_probers_addresses.find(prefix);
	if (i == m_probers_addresses.end())
		return false;

	address = i->second.first;
	port = i->second.second;
	return true;
}

#ifndef NO_LUA

bool Szbase::CompileLuaFormula(const std::wstring& formula, std::wstring& error) {
	lua_State* lua = Lua::GetInterpreter();
	bool r = lua::compile_lua_formula(lua, (const char*)SC::S2U(formula).c_str());
	if (r == false)
		error = SC::lua_error2szarp(lua_tostring(lua, -1));
	lua_pop(lua, 1);
	return r;

}

#endif

void Szbase::AddParamMonitor(SzbParamObserver *observer, const std::vector<TParam*>& params) {
#if 0
//TODO: kick it out
	std::vector<std::pair<TParam*, std::vector<std::string> > > to_monitor;
	for (std::vector<TParam*>::const_iterator i = params.begin(); 
			i != params.end();
			i++) {

		std::vector<std::string> paths;
		std::vector<TParam*> to_process, seen;

		to_process.push_back(*i);
		seen.push_back(*i);

		while (to_process.size()) {
			TParam* p = to_process.back();
			to_process.pop_back();

			if (p->GetType() == ParamType::REAL) {
				std::string path(SC::S2A((m_szarp_dir / p->GetSzarpConfig()->GetPrefix() / L"szbase" / p->GetSzbaseName()).file_string()));
				paths.push_back(path);
				continue;
			}

			assert(p->GetType() == ParamType::LUA);
			std::vector<std::wstring> strings;
			if (!extract_strings_from_formula(p->GetFormula(), strings))
				continue;

			for (std::vector<std::wstring>::iterator i = strings.begin();
					i != strings.end();
					i++) {
				std::pair<szb_buffer_t*, TParam*> bp;
				if (!FindParam(*i, bp))
					continue;

				if (std::find(seen.begin(), seen.end(), bp.second) != seen.end())
					continue;

				seen.push_back(bp.second);
				to_process.push_back(bp.second);
			}
		}
		to_monitor.push_back(std::make_pair(*i, paths));
	}
	m_monitor->add_observer(observer, to_monitor, 0);
#endif
}

#ifndef NO_LUA

#if 0
static void lua_printstack(const char* a, lua_State *lua) {
	int n = lua_gettop(lua); //number of arguments
	fprintf(stderr, "%s stack size %d\n", a, n);
	for (int i = 1; i <= n; i++)
	{
		int type = lua_type(lua, i);
		fprintf(stderr, "Stack (%d) val: %s\n", i , lua_typename(lua, type));
	}

}
#endif

template<typename... Args> int lua_fail_with_error(lua_State* lua, const char* format_string, Args... args) {
	luaL_where(lua, 1);
	lua_pushfstring(lua, format_string, args...);
	lua_concat(lua, 2);
	return lua_error(lua);
}

int lua_szbase(lua_State *lua) {
		Szbase* szbase = Szbase::GetObject();

		const unsigned char* param = (unsigned char*) luaL_checkstring(lua, 1);
		time_t time = static_cast<time_t>(lua_tonumber(lua, 2));
		SZARP_PROBE_TYPE probe_type = static_cast<SZARP_PROBE_TYPE>((int)lua_tonumber(lua, 3));
		int custom_length = lua_tointeger(lua, 4);

		time = szb_round_time(time, probe_type, custom_length);

		bool fixed = true;
		bool ok = true;
		std::wstring error;
		//FIXME: proper type
		double result = szbase->GetValue(param, time, probe_type, custom_length, &fixed, ok, error);

		assert(Lua::fixed.size() > 0);
		Lua::fixed.top() = Lua::fixed.top() && fixed;

		if (ok) {
			lua_pushnumber(lua, result);
			return 1;
		} else {
			return lua_fail_with_error(lua, "%s", SC::S2U(error).c_str());
		}
	
	}

static Integrator::Cache integrator_cache_10s;
static Integrator::Cache integrator_cache_10m;

int lua_szbase_hoursum(lua_State *lua) {
	Szbase* szbase = Szbase::GetObject();

	const unsigned char* param = (unsigned char*) luaL_checkstring(lua, 1);
	time_t start_time = static_cast<time_t>(lua_tonumber(lua, 2));
	time_t end_time = static_cast<time_t>(lua_tonumber(lua, 3));
	const SZARP_PROBE_TYPE query_probe_type = static_cast<SZARP_PROBE_TYPE>((int)lua_tonumber(lua, 4));
	const int custom_length = lua_tointeger(lua, 5);

	bool param_found = false;
	const time_t last_time = szbase->SearchLast(param, param_found);
	if (!param_found) {
		return lua_fail_with_error(lua, "Integrated param not found: '%s'", param);
	}
	if (end_time > last_time) {
		end_time = last_time;
	}
	start_time = szb_round_time(start_time, query_probe_type, custom_length);
	end_time = szb_round_time(end_time, query_probe_type, custom_length);

	Integrator::Cache& cache = query_probe_type < PT_MIN10 ? integrator_cache_10s : integrator_cache_10m;

	// time is already rounded, cache selected, and we don't want to operate on
	// types different than the ones corresponding to data in the database
	const SZARP_PROBE_TYPE db_probe_type =
		query_probe_type < PT_MIN10 ? PT_SEC10 : PT_MIN10;

	bool fixed = true;
	bool ok = true;
	std::wstring error;

	auto data_provider = [&fixed, &ok, &error, &szbase, &custom_length, &db_probe_type](const std::string& param, time_t probe_time) -> double {
		return szbase->GetValue((const unsigned char*)param.c_str(), probe_time, db_probe_type, custom_length, &fixed, ok, error);
	};
	auto time_mover = [&db_probe_type](time_t start_time, int steps) -> time_t {
		// here db_probe_type must point to final type (as used by integrator)
		return szb_move_time(start_time, steps, db_probe_type);
	};
	auto is_no_data = [](double value) -> bool {
		return IS_SZB_NODATA(value);
	};

	// create integrator with external static cache every time, as data_provider uses local variables
	Integrator integrator(data_provider, time_mover, is_no_data, SZB_NODATA, cache);
	const double integral = integrator.GetIntegral((const char*)param, start_time, end_time);
	const double integral_1h = integral / 3600;	// s -> h

	assert(Lua::fixed.size() > 0);
	Lua::fixed.top() = Lua::fixed.top() && fixed;

	if (ok) {
		lua_pushnumber(lua, integral_1h);
		return 1;
	} else {
		return lua_fail_with_error(lua, "%s", SC::S2U(error).c_str());
	}

}

int lua_szbase_search_from_reference(lua_State *lua, const unsigned char* param, const time_t reference_time, const time_t limit_time, const int direction);

int lua_szbase_search_left(lua_State *lua) {
	const unsigned char* param = (unsigned char*) luaL_checkstring(lua, 1);
	bool param_found = false;
	const time_t first_time = Szbase::GetObject()->SearchFirst(param, param_found);
	if (!param_found) {
		return lua_fail_with_error(lua, "Search left param not found: '%s'", param);
	}
	time_t end_time = static_cast<time_t>(lua_tonumber(lua, 2));
	return lua_szbase_search_from_reference(lua, param, end_time, first_time, -1);
}

int lua_szbase_search_right(lua_State *lua) {
	const unsigned char* param = (unsigned char*) luaL_checkstring(lua, 1);
	bool param_found = false;
	const time_t last_time = Szbase::GetObject()->SearchLast(param, param_found);
	if (!param_found) {
		return lua_fail_with_error(lua, "Search right param not found: '%s'", param);
	}
	time_t start_time = static_cast<time_t>(lua_tonumber(lua, 2));
	return lua_szbase_search_from_reference(lua, param, start_time, last_time, 1);
}

int lua_szbase_search_from_reference(lua_State *lua, const unsigned char* param, const time_t reference_time, const time_t limit_time, const int direction) {
	const SZARP_PROBE_TYPE query_probe_type = static_cast<SZARP_PROBE_TYPE>((int)lua_tonumber(lua, 3));
	bool probe_time_found = false;
	std::wstring search_error{};
	// we delegate timestamp validation to Search(), basing on r_t, l_t and direction
	const time_t probe_time = Szbase::GetObject()->Search(SC::U2S(param), reference_time, limit_time, direction, query_probe_type, probe_time_found, search_error);
	if (!probe_time_found) {
		return lua_fail_with_error(lua, "Search from reference failed on param: '%s', error: '%s'", param, search_error.c_str());
	}
	lua_pushnumber(lua, probe_time);
	return 1;
}

int lua_szbase_move_time(lua_State* lua) {
	time_t time = static_cast<time_t>(lua_tonumber(lua, 1));
	int count = lua_tointeger(lua, 2);
	SZARP_PROBE_TYPE probe_type = static_cast<SZARP_PROBE_TYPE>((int)lua_tonumber(lua, 3));
	int custom_lenght = lua_tointeger(lua, 4);

	time_t result = szb_move_time(time , count, probe_type, custom_lenght);
	lua_pushnumber(lua, result);

	return 1;
}

int lua_szbase_round_time(lua_State* lua) {
	time_t time = static_cast<time_t>(lua_tonumber(lua, 1));
	SZARP_PROBE_TYPE probe_type = static_cast<SZARP_PROBE_TYPE>((int)lua_tonumber(lua, 2));
	int custom_length = lua_tointeger(lua, 3);

	time_t result = szb_round_time(time , probe_type, custom_length);
	lua_pushnumber(lua, result);

	return 1;
}

int lua_szbase_isnan(lua_State* lua) {
	double v = lua_tonumber(lua, 1);
	lua_pushboolean(lua, std::isnan(v));
	return 1;
}

int lua_szbase_search_first(lua_State *lua) {
	const unsigned char* param = (unsigned char*) luaL_checkstring(lua, 1);
	Szbase* szbase = Szbase::GetObject();

	bool ok;
	time_t ret = szbase->SearchFirst(SC::U2S(param), ok);

	if (ok) {
		lua_pushnumber(lua, double(ret));
		return 1;
	} else {
		return luaL_error(lua, "Param %s not found", param);
	}
}

int lua_szbase_search_last(lua_State *lua) {
	const unsigned char* param = (unsigned char*) luaL_checkstring(lua, 1);
	Szbase* szbase = Szbase::GetObject();
	
	bool ok;
	time_t ret = szbase->SearchLast(param, ok);

	if (ok) {
		lua_pushnumber(lua, double(ret));
		return 1;
	} else {
		return luaL_error(lua, "Param %s not found", param);
	}
}

int lua_szbase_in_season(lua_State *lua) {
	const unsigned char* prefix = reinterpret_cast<const unsigned char*>(luaL_checkstring(lua, 1));
	time_t time = static_cast<time_t>(lua_tonumber(lua, 2));
	IPKContainer *ic = IPKContainer::GetObject();
	TSzarpConfig *cfg = ic->GetConfig(SC::U2S(prefix));

	if (cfg == NULL)
		luaL_error(lua, "Config %s not found", (char*)prefix);

	lua_pushboolean(lua, cfg->GetSeasons()->IsSummerSeason(time));

	return 1;

}

int lua_szbase_nan(lua_State* lua) {
	lua_pushnumber(lua, nan(""));
	return 1;
}


static int RegisterSzbaseFuncs(lua_State *lua) {
	const struct luaL_reg SzbaseLibFun[] = {
		{ "szbase", lua_szbase },
		{ "szbase_hoursum", lua_szbase_hoursum },
		{ "szb_move_time", lua_szbase_move_time },
		{ "szb_round_time", lua_szbase_round_time },
		{ "szb_search_first", lua_szbase_search_first },
		{ "szb_search_last", lua_szbase_search_last },
		{ "isnan", lua_szbase_isnan },
		{ "nan", lua_szbase_nan },
		{ "in_season", lua_szbase_in_season },
		{ NULL, NULL }
	};

	const luaL_Reg *lib = SzbaseLibFun;

	for (; lib->func; lib++) 
		lua_register(lua, lib->name, lib->func);

	return 0;
}
	

lua_State* Lua::lua = NULL;

void Lua::Init() {
	assert(lua == NULL);

	lua = lua_open();
	luaL_openlibs(lua);

	lua::set_probe_types_globals(lua);
	RegisterSzbaseFuncs(lua);

}

lua_State* Lua::GetInterpreter() {
	if (lua == NULL)
		Init();
	return lua;
}

std::stack<bool> Lua::fixed;

#endif
