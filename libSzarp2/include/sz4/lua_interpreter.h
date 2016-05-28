#ifndef __SZ4_LUA_INTERPRETER_H__
#define __SZ4_LUA_INTERPRETER_H__
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

#include "sz4/defs.h"
#include "sz4/time.h"

namespace sz4 {

template<class base> class lua_interpreter {
	lua_State* m_lua;
public:
	lua_interpreter();

	bool prepare_param(TParam* param);

	double calculate_value(nanosecond_time_t start, SZARP_PROBE_TYPE probe_type, int custom_length);

	void pop_prepared_param();

	void initialize(base* _base, typename base::ipk_container_type* container);

	lua_State* lua();

	~lua_interpreter();

	static const int lua_base_ipk_pair_key;
};

}

#endif
