#ifndef __SZ4_DEFS_H__
#define __SZ4_DEFS_H__
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

#include <limits>
#include <cmath>
#include <stdint.h>

#include "sz4/time.h"

namespace sz4 {

template<class V, class T> struct value_time_pair {
	typedef V value_type;
	typedef T time_type;

	value_type value;
	time_type  time;
} __attribute__ ((packed));

template<class T> bool value_is_no_data(const T& v) {
	return v == std::numeric_limits<T>::min();
}

template<> bool value_is_no_data<double>(const double& v) {
	return std::isnan(v);
}

template<> bool value_is_no_data<float>(const float& v) {
	return std::isnan(v);
}

template<class T> struct time_difference { };

template<> struct time_difference<second_time_t> {
	typedef long type;
};

template<> struct time_difference<nanosecond_time_t> {
	typedef long long type;
};

template<class T> struct value_sum { };

template<> struct value_sum<short> {
	typedef int type;
};

template<> struct value_sum<int> {
	typedef long long type;
};

template<> struct value_sum<unsigned int> {
	typedef unsigned long long type;
};

template<> struct value_sum<float> {
	typedef double type;
};

template<> struct value_sum<double> {
	typedef double type;
};

}

#endif
