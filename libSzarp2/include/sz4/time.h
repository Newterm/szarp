#ifndef __SZ4_TIME_H__
#define __SZ4_TIME_H__
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

#include <stdint.h>

namespace sz4 {

typedef uint32_t second_time_t;

struct nanosecond_time_t {
	uint32_t second;
	uint32_t nanosecond;
} __attribute__ ((packed));

long long operator-(const nanosecond_time_t& t1, const nanosecond_time_t& t2);

bool operator==(const nanosecond_time_t& t1, const nanosecond_time_t& t2);

template<class T> class invalid_time_value { };

template<> class invalid_time_value<nanosecond_time_t> {
public:
	static bool is_valid(const nanosecond_time_t& t) { return !(t == value()); }
	static nanosecond_time_t value() { nanosecond_time_t v = { -1, -1 }; return v; }
};

template<> class invalid_time_value<second_time_t> {
public:
	static bool is_valid(const second_time_t& t) { return !(t == value()); }
	static second_time_t value() { return -1; }
};

}

#endif
