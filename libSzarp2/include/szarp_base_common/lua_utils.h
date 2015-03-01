#ifndef __LUA_UTILS_H__
#define __LUA_UTILS_H__
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

#include <lua.hpp>

class TParam;

namespace lua {

void set_probe_types_globals(lua_State *lua);

bool prepare_param(lua_State *lua, TParam* param);

int  compile_lua_param(lua_State *lua, TParam *p);

bool compile_lua_formula(lua_State *lua, const char *formula, const char *formula_name = "param_fomula", bool ret_v_val = true);

}

#endif
