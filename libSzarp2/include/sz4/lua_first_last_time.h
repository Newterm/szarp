#ifndef __SZ4_LUA_FIRST_LAST_TIME_H__
#define __SZ4_LUA_FIRST_LAST_TIME_H__
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

#include "sz4/buffer.h"

namespace sz4 {

template<class T> void lua_adjust_first_time(TParam* param, T& t) {
	if (param->GetLuaStartDateTime() > 0)
		t = T(second_time_t(param->GetLuaStartDateTime()));
	else if (time_trait<T>::is_valid(t))
		t = T(szb_move_time(t, param->GetLuaStartOffset(), PT_SEC));
}

template<class T> void lua_adjust_last_time(TParam* param, T& t) {
	if (time_trait<T>::is_valid(t))
		t = szb_move_time(t, param->GetLuaEndOffset(), PT_SEC);
}

}

#endif
