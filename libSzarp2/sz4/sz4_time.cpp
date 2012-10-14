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

#include "szarp_base_common/time.h"
#include "sz4/time.h"

namespace sz4 {

const nanosecond_time_t invalid_time_value<nanosecond_time_t>::value = make_nanosecond_time(-1, -1);

const second_time_t invalid_time_value<second_time_t>::value = -1;

nanosecond_time_t make_nanosecond_time(uint32_t second, uint32_t nanosecond) {
	return nanosecond_time_t(second, nanosecond);
}

nanosecond_time_t 
szb_move_time(const nanosecond_time_t& t, int count, SZARP_PROBE_TYPE probe_type, 
		int custom_length) {
	return make_nanosecond_time(szb_move_time(time_t(t.second), count, probe_type, custom_length), t.nanosecond);
}

nanosecond_time_t 
szb_round_time(nanosecond_time_t t, SZARP_PROBE_TYPE probe_type, int custom_length) {
	return make_nanosecond_time(szb_round_time(time_t(t.second), probe_type, custom_length), 0);
}

}
