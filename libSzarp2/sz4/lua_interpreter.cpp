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

#include "sz4/types.h"
#include "sz4/defs.h"
#include "sz4/block_cache.h"
#include "sz4/lua_interpreter.h"
#include "sz4/lua_interpreter_templ.h"

namespace sz4 {

int lua_sz4_move_time(lua_State* lua) {
	nanosecond_time_t time(lua_tonumber(lua, 1));
	int count(lua_tointeger(lua, 2));
	SZARP_PROBE_TYPE probe_type(static_cast<SZARP_PROBE_TYPE>((int)lua_tonumber(lua, 3)));
	int custom_lenght(lua_tointeger(lua, 4));

	lua_pushnumber(lua, szb_move_time(time, count, probe_type, custom_lenght));
	return 1;
}

int lua_sz4_round_time(lua_State* lua) {
	nanosecond_time_t time(lua_tonumber(lua, 1));
	SZARP_PROBE_TYPE probe_type = static_cast<SZARP_PROBE_TYPE>((int)lua_tonumber(lua, 2));
	int custom_length = lua_tointeger(lua, 3);

	nanosecond_time_t result = szb_round_time(time , probe_type, custom_length);
	lua_pushnumber(lua, result);
	return 1;
}


int lua_sz4_isnan(lua_State* lua) {
	double v = lua_tonumber(lua, 1);
	lua_pushboolean(lua, std::isnan(v));
	return 1;
}

int lua_sz4_nan(lua_State* lua) {
	lua_pushnumber(lua, nan(""));
	return 1;
}

template class lua_interpreter<base_types>;

}
