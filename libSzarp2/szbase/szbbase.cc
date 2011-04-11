/* 
  SZARP: SCADA software 

*/
/*
 * szbase - szbbase.c - main public interfaces
 * $Id$
 * <pawel@praterm.com.pl>
 */

#include <assert.h>
#include <algorithm>
#include <sstream>
#include <vector>
#include <time.h>

#include <liblog.h>
#include "szbbase.h"
#include "szbbuf.h"
#include "szbhash.h"

#include "cacheabledatablock.h"
#include "conversion.h"
#include "proberconnection.h"

#ifdef MINGW32
#include "mingw32_missing.h"
#undef GetObject
#endif

#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
#include "loptcalculate.h"
#endif
#endif

using std::find;
using std::string;
using std::pair;
using std::tr1::unordered_map;

const int default_szbuffer_cache_size = 12 * 12 * 5 * 2;	/** 12 graphs * 12 months * 5 block for param and make it double*/

const int default_maximum_search_time = 15;			/** maximum deafult serach time */

Szbase::Szbase(const std::wstring& szarp_dir) : m_szarp_dir(szarp_dir), m_config_modification_callback(NULL)
{
	m_current_query = 0;
	m_maximum_search_time = default_maximum_search_time;
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
	m_file_watcher.Terminate();
	for (TBI::iterator i = m_ipkbasepair.begin() ; i != m_ipkbasepair.end() ; ++i) {
		std::pair<szb_buffer_t*, TSzarpConfig*> v = i->second;
		szb_free_buffer(v.first);
	}
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

	return AddBase((m_szarp_dir / prefix / L"szbase").file_string(), prefix);
}

void Szbase::AddExtraParam(const std::wstring &prefix, TParam *param) {

	m_extra_params[prefix].insert(param);
	TBI::iterator i = m_ipkbasepair.find(prefix);
	if (i != m_ipkbasepair.end())
		AddParamToHash(prefix, param);

}

#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
void Szbase::AddLuaOptParamReference(TParam* refered, TParam* referring) {
	m_lua_opt_param_reference_map[refered].insert(referring);
}
#endif
#endif

void Szbase::RemoveExtraParam(const std::wstring& prefix, TParam *param) {

	m_extra_params[prefix].erase(param);

	TBI::iterator iibk = m_ipkbasepair.find(prefix);
	if (iibk == m_ipkbasepair.end())
		return;

#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
	std::set<TParam*>& set = m_lua_opt_param_reference_map[param];
	for (std::set<TParam*>::iterator i = set.begin();
			i != set.end();
			i++) {
		TParam* p = *i;
		const std::wstring& prefix = p->GetSzarpConfig()->GetPrefix();
		std::pair<szb_buffer_t*, TSzarpConfig*>& bc = m_ipkbasepair[prefix];
		bc.first->RemoveExecParam(p);
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
		iibk->second.first->RemoveExecParam(param);
	}
#endif
#endif

	std::wstring global_param_name = prefix + L":" + param->GetName();
	std::wstring translated_global_param_name = prefix + L":" + param->GetTranslatedName();

	TBP::iterator j = m_params.find(global_param_name);
	if (j == m_params.end()) {
		return;
	}

	m_params.erase(global_param_name);
	m_params.erase(translated_global_param_name);

	m_u8params.erase(SC::S2U(global_param_name));
	m_u8params.erase(SC::S2U(translated_global_param_name));

	ClearParamFromCache(prefix, param);
}

void Szbase::AddParamToHash(const std::wstring& prefix, TParam *param) {

	std::pair<szb_buffer_t*, TSzarpConfig*> bt = m_ipkbasepair[prefix];

	std::wstring global_param_name = prefix + L":" + param->GetName();
	m_params[global_param_name] = pair<szb_buffer_t*, TParam*>(bt.first, param);

	std::wstring translated_global_param_name = prefix + L":" + param->GetTranslatedName();
	m_params[translated_global_param_name] = pair<szb_buffer_t*, TParam*>(bt.first, param);

	m_u8params[SC::S2U(global_param_name)] = pair<szb_buffer_t*, TParam*>(bt.first, param);
	m_u8params[SC::S2U(translated_global_param_name)] = pair<szb_buffer_t*, TParam*>(bt.first, param);

}

bool Szbase::AddBase(const std::wstring& szbase_dir, const std::wstring &prefix) {

	IPKContainer *ipks = IPKContainer::GetObject();
	TSzarpConfig *ipk = ipks->GetConfig(prefix);
	if (ipk == NULL) {
		sz_log(0, "Unable to load ipk dir:%ls", prefix.c_str());
		return false;
	}
		
	szb_buffer_t* szbbuf = szb_create_buffer(this, szbase_dir, m_buffer_cache_size, ipk);
	if (szbbuf == NULL) {
		sz_log(0, "Unable to load ipk dir:%ls", szbase_dir.c_str());
		return false;
	}

	m_ipkbasepair[prefix] = pair<szb_buffer_t*, TSzarpConfig*>(szbbuf, ipk);

	for (TParam *p = ipk->GetFirstParam(); p; p = p->GetNext(true))
		AddParamToHash(prefix, p);

	for (std::set<TParam*>::iterator i = m_extra_params[prefix].begin(); i != m_extra_params[prefix].end(); i++)
		AddParamToHash(prefix, *i);

	if ( m_config_modification_callback != NULL ) {
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

	TBI::iterator bi = m_ipkbasepair.find(prefix);
	if (bi == m_ipkbasepair.end())
		return;

	m_file_watcher.RemoveFileWatch(prefix);

	szb_buffer_t *b = bi->second.first;

	std::vector<std::wstring> pn;
	for (TBP::iterator i = m_params.begin() ; i != m_params.end(); ++i)
		if (i->second.first == b)
			pn.push_back(i->first);

	for (std::vector<std::wstring>::iterator i = pn.begin(); i != pn.end(); ++i) {
		m_params.erase(*i);
		m_u8params.erase(SC::S2U(*i));
	}

	m_ipkbasepair.erase(prefix);

	if (poison_cache)
		b->cachepoison = true;
	szb_free_buffer(b);

	m_ipkbasepair.erase(prefix);
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
	TBPU8::iterator i = m_u8params.find(param);

	if (i == m_u8params.end()) {
		std::wstring prefix = SC::U2S(param.substr(0, param.find(':')));
		if (m_ipkbasepair.find(prefix) != m_ipkbasepair.end())
			return false;
		if (AddBase(prefix))
			return FindParam(param, bp);
		else
			return false;
	}

	bp = i->second;
	return true;
}

bool Szbase::FindParam(const std::wstring& param, std::pair<szb_buffer_t*, TParam*>& bp) {
	TBP::iterator i = m_params.find(param);

	if (i == m_params.end()) {
		std::wstring prefix = param.substr(0, param.find(L':'));
		if (m_ipkbasepair.find(prefix) != m_ipkbasepair.end())
			return false;
		if (AddBase(prefix))
			return FindParam(param, bp);
		else
			return false;
	}

	bp = i->second;
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

szb_buffer_t* Szbase::GetBuffer(const std::wstring &prefix) {

	TBI::iterator i = m_ipkbasepair.find(prefix);

	if (i == m_ipkbasepair.end()) {
		AddBase(prefix);
		i = m_ipkbasepair.find(prefix);
	}

	return i->second.first;
}

void Szbase::NextQuery() {
	if (m_current_query == std::numeric_limits<long>::max())
		m_current_query = 0;

	m_current_query++;
}

void Szbase::SetProberAddress(std::wstring prefix, std::wstring address, std::wstring port) {
	m_probers_addresses[prefix] = std::make_pair(address, port);

	TBI::iterator i = m_ipkbasepair.find(prefix);
	if (i != m_ipkbasepair.end()) {
		if (i->second.first->prober_connection) {
			delete i->second.first->prober_connection;
			i->second.first->prober_connection = NULL;
		}
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
	bool r = szb_compile_lua_formula(lua, (const char*)SC::S2U(formula).c_str());
	if (r == false)
		error = SC::U2S((unsigned char*)lua_tostring(lua, -1));
	lua_pop(lua, 1);
	return r;

}

#endif

#ifndef NO_LUA

bool szb_compile_lua_formula(lua_State *lua, const char *formula, const char *formula_name, bool ret_v_val) {

	std::ostringstream paramfunction;

	using std::endl;

	paramfunction					<< 
	"return function ()"				<< endl <<
	"	local p = szbase"			<< endl <<
	"	local PT_MIN10 = ProbeType.PT_MIN10"	<< endl <<
	"	local PT_HOUR = ProbeType.PT_HOUR"	<< endl <<
	"	local PT_HOUR8 = ProbeType.PT_HOUR8"	<< endl <<
	"	local PT_DAY = ProbeType.PT_DAY"	<< endl <<
	"	local PT_WEEK = ProbeType.PT_WEEK"	<< endl <<
	"	local PT_MONTH = ProbeType.PT_MONTH"	<< endl <<
	"	local PT_YEAR = ProbeType.PT_YEAR"	<< endl <<
	"	local PT_CUSTOM = ProbeType.PT_CUSTOM"	<< endl <<
	"	local szb_move_time = szb_move_time"	<< endl <<
	"	local state = {}"			<< endl <<
	"	return function (t,pt)"			<< endl;

	if (ret_v_val)
		paramfunction <<
		"		local v = nil"		<< endl;

	paramfunction << formula			<< endl;

	if (ret_v_val)
		paramfunction << 
		"		return v"		<< endl;

	paramfunction <<
	"	end"					<< endl <<
	"end"						<< endl;

	string str = paramfunction.str();

	const char* content = str.c_str();

	int ret = luaL_loadbuffer(lua, content, strlen(content), formula_name);
	if (ret != 0)
		return false;

	ret = lua_pcall(lua, 0, 1, 0);
	if (ret != 0)
		return false;

	ret = lua_pcall(lua, 0, 1, 0);
	if (ret != 0)
		return false;

	return true;
}


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

		if (ok == false)
			luaL_error(lua, "%s", SC::S2U(error).c_str());

		lua_pushnumber(lua, result);

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
		luaL_error(lua, "Param %ls not found", SC::U2S(param).c_str());
		return 0;
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
		luaL_error(lua, "Param %ls not found", SC::U2S(param).c_str());
		return 0;
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
	



static int InitProbeType(lua_State *lua) {
	lua_newtable(lua);
#define LUA_ENUM(name) \
	lua_pushlstring(lua, #name, sizeof(#name)-1); \
	lua_pushnumber(lua, name); \
	lua_settable(lua, -3);
	
	LUA_ENUM(PT_SEC10);
	LUA_ENUM(PT_MIN10);
	LUA_ENUM(PT_HOUR);
	LUA_ENUM(PT_HOUR8);
	LUA_ENUM(PT_DAY);
	LUA_ENUM(PT_WEEK);
	LUA_ENUM(PT_MONTH);
	LUA_ENUM(PT_YEAR);
	LUA_ENUM(PT_CUSTOM);
	
	lua_setglobal(lua, "ProbeType");
	
	return 0;
	
}

lua_State* Lua::lua = NULL;

void Lua::Init() {
	assert(lua == NULL);

	lua = lua_open();
	luaL_openlibs(lua);

	InitProbeType(lua);
	RegisterSzbaseFuncs(lua);

}

lua_State* Lua::GetInterpreter() {
	if (lua == NULL)
		Init();
	return lua;
}

std::stack<bool> Lua::fixed;

#endif
