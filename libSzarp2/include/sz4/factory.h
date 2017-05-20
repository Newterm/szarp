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

#ifndef __SZ4_FACTORY_H__
#define __SZ4_FACTORY_H__

namespace sz4 {

template<class return_type, class builder> struct factory {
	template<class ...Args> static return_type* op(TParam *param, Args... args) {
		switch (param->GetDataType()) {
			case TParam::SHORT:
				return op_1<short, Args...>(param, args...);
			case TParam::INT:
				return op_1<int, Args...>(param, args...);
			case TParam::FLOAT:
				return op_1<float, Args...>(param, args...);
			case TParam::DOUBLE:
				return op_1<double, Args...>(param, args...);
			default:
				assert(false);
		}
		return NULL;
	};
private:
	template<class _data, class ...Args> static return_type* op_1(TParam* param, Args... args) {
		switch (param->GetTimeType()) {
			case TParam::SECOND:
				return op_2<_data, second_time_t, Args...>(args...);
			case TParam::NANOSECOND:
				return op_2<_data, nanosecond_time_t, Args...>(args...);
			default:
				assert(false);
		}
		return NULL;
	}

	template<class _data, class _time, class ...Args> static return_type* op_2(Args... args) {
		return builder::template op<_data, _time>(args...);
	}
};

}
#endif
