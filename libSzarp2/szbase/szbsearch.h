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
#ifndef SZB_SERACH_H__
#define SZB_SERACH_H__

time_t
szb_real_search_data(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction);

time_t
szb_combined_search_data(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction);

time_t
szb_definable_search_data(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction);

time_t
szb_lua_search_data(szb_buffer_t * buffer, TParam * param , time_t start, time_t end, int direction);

time_t 
szb_real_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle);

time_t 
szb_combined_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle);

time_t 
szb_definable_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle);

time_t 
szb_lua_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle);

time_t
szb_lua_search_by_value(szb_buffer_t* buffer, TParam* param, SZARP_PROBE_TYPE probe_type, time_t start_time, time_t end_time, int direction);

bool
szb_lua_search_first_last_date(szb_buffer_t* buffer, TParam* param, SZARP_PROBE_TYPE probe_type, time_t& first_date, time_t& last_date);

#endif
