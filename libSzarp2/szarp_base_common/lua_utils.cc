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

#include <sstream>
#include <cstring>

#include "conversion.h"
#include "szarp_config.h"
#include "liblog.h"
#include "szarp_base_common/defines.h"
#include "szarp_base_common/lua_utils.h"

namespace lua {

void set_probe_types_globals(lua_State *lua) {
	lua_newtable(lua);
#define LUA_ENUM(name) \
	lua_pushlstring(lua, #name, sizeof(#name)-1); \
	lua_pushnumber(lua, name); \
	lua_settable(lua, -3);
	
	LUA_ENUM(PT_SEC);
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
} 

bool prepare_param(lua_State *lua, TParam* param) {
	int ref = param->GetLuaParamReference();
	if (ref == LUA_NOREF) {
		ref = compile_lua_param(lua, param);
		param->SetLuaParamRef(ref);
	}

	if (ref != LUA_REFNIL) {
		lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);
		return true;
	} else {
		return false;
	}
}

int compile_lua_param(lua_State *lua, TParam *p) {
	int lua_function_reference = LUA_NOREF;
	if (compile_lua_formula(lua, (const char*) p->GetLuaScript(), (const char*)SC::S2U(p->GetName()).c_str(), true))
		lua_function_reference = luaL_ref(lua, LUA_REGISTRYINDEX);
	else {
		sz_log(0, "Error compiling param %ls: %s\n", p->GetName().c_str(), lua_tostring(lua, -1));
		lua_function_reference = LUA_REFNIL;
	}
	p->SetLuaParamRef(lua_function_reference);
	return lua_function_reference;

}

bool compile_lua_formula(lua_State *lua, const char *formula, const char *formula_name, bool ret_v_val) {

	std::ostringstream paramfunction;

	using std::endl;

	paramfunction					<< 
	"return function ()"				<< endl <<
	"	local p = szbase"			<< endl <<
	"	local PT_SEC = ProbeType.PT_SEC"	<< endl <<
	"	local PT_SEC10 = ProbeType.PT_SEC10"	<< endl <<
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

	std::string str = paramfunction.str();

	const char* content = str.c_str();

	int ret = luaL_loadbuffer(lua, content, std::strlen(content), formula_name);
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

}

