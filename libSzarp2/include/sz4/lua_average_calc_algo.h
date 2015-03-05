#ifndef __SZ4_LUA_AVERAGE_CALC_ALGO_H_
#define __SZ4_LUA_AVERAGE_CALC_ALGO_H_
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

namespace sz4 {

class lua_average_calc_algo {
public:
	virtual void initialize() = 0;
	std::tr1::tuple<double, bool> calculate_value(second_time_t time, SZARP_PROBE_TYPE probe_type) {
		initialize();
		std::tr1::tuple<double, bool> ret(nan(""), true);
		do_calculate_value(time, probe_type, std::tr1::get<0>(ret), std::tr1::get<1>(ret));
		return ret;
	}

	virtual void do_calculate_value(second_time_t time, SZARP_PROBE_TYPE probe_type, double &result, bool& fixed) = 0;
	
};

}

#endif
