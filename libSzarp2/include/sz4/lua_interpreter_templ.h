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

#include "conversion.h"

#include "szarp_base_common/lua_utils.h"
#include "sz4/types.h"
#include "sz4/exceptions.h"
#include "sz4/base.h"
#include "sz4/util.h"

namespace sz4 {

template<class types> struct base_ipk_pair : public std::pair<base_templ<types>*, typename types::ipk_container_type*> {};

template<class types> base_ipk_pair<types>* get_base_ipk_pair(lua_State *lua) {
	base_ipk_pair<types>* ret;
	lua_pushlightuserdata(lua, (void*)&lua_interpreter<types>::lua_base_ipk_pair_key);
	lua_gettable(lua, LUA_REGISTRYINDEX);
	ret = (base_ipk_pair<types>*)lua_touserdata(lua, -1);
	lua_pop(lua, 1);
	return ret;
}

template<class types> void push_base_ipk_pair(lua_State *lua, base_templ<types>* base, typename types::ipk_container_type* container) {
	lua_pushlightuserdata(lua, (void*)&lua_interpreter<types>::lua_base_ipk_pair_key);

	base_ipk_pair<types>* pair = (base_ipk_pair<types>*)lua_newuserdata(lua, sizeof(base_ipk_pair<types>));
	pair->first = base;
	pair->second = container;

	lua_settable(lua, LUA_REGISTRYINDEX);
}

template<class base_types> int lua_sz4(lua_State *lua) {
	const unsigned char* param_name = (unsigned char*) luaL_checkstring(lua, 1);
	if (param_name == NULL) 
                 luaL_error(lua, "Invalid param name");

	nanosecond_time_t time(lua_tonumber(lua, 2));
	SZARP_PROBE_TYPE probe_type(static_cast<SZARP_PROBE_TYPE>((int)lua_tonumber(lua, 3)));

	base_ipk_pair<base_types>* base_ipk = get_base_ipk_pair<base_types>(lua);
	TParam* param = base_ipk->second->GetParam(std::basic_string<unsigned char>(param_name));
	if (param == NULL)
                 luaL_error(lua, "Param %s not found", param_name);

	weighted_sum<double, nanosecond_time_t> sum;
	base_ipk->first->get_weighted_sum(param, time, szb_move_time(time, 1, SZARP_PROBE_TYPE(probe_type), 0), probe_type, sum);
	base_ipk->first->fixed_stack().top().first
			&= sum.fixed();
	base_ipk->first->fixed_stack().top().second.insert(
				sum.refferred_blocks().begin(),
				sum.refferred_blocks().end());

	lua_pushnumber(lua, scale_value(sum.avg(), param));
	return 1;
}

int lua_sz4_move_time(lua_State* lua);

int lua_sz4_round_time(lua_State* lua);

template<class types> int lua_sz4_in_season(lua_State *lua) {
	const unsigned char* prefix = reinterpret_cast<const unsigned char*>(luaL_checkstring(lua, 1));
	time_t time(lua_tonumber(lua, 2));
	base_ipk_pair<types>* base_ipk = get_base_ipk_pair<types>(lua);

	TSzarpConfig *cfg = base_ipk->second->GetConfig(SC::U2S(prefix));
	if (cfg == NULL)
		luaL_error(lua, "Config %s not found", (char*)prefix);
	lua_pushboolean(lua, cfg->GetSeasons()->IsSummerSeason(time));
	return 1;
}

int lua_sz4_isnan(lua_State* lua);

int lua_sz4_nan(lua_State* lua);

template<class types> lua_interpreter<types>::lua_interpreter() {
	m_lua = lua_open();
	if (m_lua == NULL) {
		throw exception("Failed to initialize lua interpreter");	
	}

	luaL_openlibs(m_lua);
	lua::set_probe_types_globals(m_lua);

	const struct luaL_reg sz4_funcs_lib[] = {
		{ "szbase", lua_sz4<types> },
		{ "szb_move_time", lua_sz4_move_time },
		{ "szb_round_time", lua_sz4_round_time },
//		{ "szb_search_first", lua_sz4_search_first },
//		{ "szb_search_last", lua_sz4_search_last },
		{ "isnan", lua_sz4_isnan },
		{ "nan", lua_sz4_nan },
		{ "in_season", lua_sz4_in_season<types> },
		{ NULL, NULL }
	};

	const luaL_Reg *lib = sz4_funcs_lib;

	for (; lib->func; lib++) 
		lua_register(m_lua, lib->name, lib->func);

};

template<class types> void lua_interpreter<types>::initialize(base_templ<types>* base, typename types::ipk_container_type* container) {
	push_base_ipk_pair(m_lua, base, container);
}

template<class types> bool lua_interpreter<types>::prepare_param(TParam* param) {
	return lua::prepare_param(m_lua, param);
}

template<class types> double lua_interpreter<types>::calculate_value(nanosecond_time_t start, SZARP_PROBE_TYPE probe_type, int custom_length) {
	double result;

	lua_pushvalue(m_lua, -1);	
	lua_pushnumber(m_lua, start);
	lua_pushnumber(m_lua, probe_type);
	lua_pushnumber(m_lua, custom_length);

	int ret = lua_pcall(m_lua, 3, 1, 0);
	if (ret != 0)
		throw exception(std::string("Lua param execution error: ") + lua_tostring(m_lua, -1));

	if (lua_isnumber(m_lua, -1))
		result = lua_tonumber(m_lua, -1);
	else if (lua_isboolean(m_lua, -1))
		result = lua_toboolean(m_lua, -1);
	else
		result = nan("");
	lua_pop(m_lua, 1);

	return result;
}

template<class types> void lua_interpreter<types>::pop_prepared_param() {
	lua_pop(m_lua, 1);
}

template<class types> lua_interpreter<types>::~lua_interpreter() {
	if (m_lua)
		lua_close(m_lua);
}

template<class types> lua_State* lua_interpreter<types>::lua() {
	return m_lua;
}

template<class types> const int lua_interpreter<types>::lua_base_ipk_pair_key = 0;

}
