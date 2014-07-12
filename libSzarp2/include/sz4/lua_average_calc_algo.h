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
	std::tr1::tuple<double, bool, std::set<generic_block*> > calculate_value(second_time_t time, SZARP_PROBE_TYPE probe_type) {
		initialize();

		second_time_t end = szb_move_time(time, 1, probe_type);
		SZARP_PROBE_TYPE step = get_probe_type_step(probe_type);

		bool fixed = true;
		double sum = 0;
		int count = 0;

		std::set<generic_block*> referred_blocks;

		while (time < end) {
			double value;
			do_calculate_value(time, step, value, referred_blocks, fixed);

			if (!std::isnan(value)) {
				sum += value;
				count += 1;
			}
		
			time = szb_move_time(time, 1, step);
		}

		return std::tr1::make_tuple(count ? (sum / count) : std::nan(""), fixed, referred_blocks);
	}

	virtual void do_calculate_value(second_time_t time, SZARP_PROBE_TYPE probe_type, double &result, std::set<generic_block*>& reffered_blocks, bool& fixed) = 0;
	
};

}

#endif
