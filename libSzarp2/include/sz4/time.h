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
#ifndef __SZ4_TIME_H__
#define __SZ4_TIME_H__

#include <stdint.h>
#include <ctime>

#include "szarp_base_common/time.h"

namespace sz4 {

typedef uint32_t second_time_t;

struct nanosecond_time_t {
	nanosecond_time_t() : second(0), nanosecond(0) {}
	nanosecond_time_t(uint32_t _second, uint32_t _nanosecond) : second(_second), nanosecond(_nanosecond) {}
	nanosecond_time_t(double _time) : second(_second), nanosecond((_time - (long)_time) * 1000000000) {}
	nanosecond_time_t(const second_time_t& time) : second(time), nanosecond(0) {}
	bool operator==(const nanosecond_time_t& t) const { return second == t.second && nanosecond == t.nanosecond; }
	long long operator- (const nanosecond_time_t& t) const {
		long long d = second;
		d -= t.second;
		d *= 1000000000;
		d += nanosecond;
		d -= t.nanosecond;
		return d;
	}
	bool operator< (const nanosecond_time_t& t) const { return second < t.second || (second == t.second && nanosecond < t.nanosecond); } 
	bool operator<= (const nanosecond_time_t& t) const { return second < t.second || (second == t.second && nanosecond <= t.nanosecond); } 
	bool operator> (const nanosecond_time_t& t) const { return second > t.second || (second == t.second && nanosecond > t.nanosecond); } 
	bool operator>= (const nanosecond_time_t& t) const { return second > t.second || (second == t.second && nanosecond >= t.nanosecond); } 
	operator second_time_t() const { return second; }
	operator double() const { return second + double(nanosecond) / 1000000000; }
	uint32_t second;
	uint32_t nanosecond;
} __attribute__ ((packed));

nanosecond_time_t make_nanosecond_time(uint32_t second, uint32_t nanosecond);

template<class T> class invalid_time_value { };

template<> class invalid_time_value<nanosecond_time_t> {
public:
	static bool is_valid(const nanosecond_time_t& t) { return !(t == value); }
	static const nanosecond_time_t value;
};

template<> class invalid_time_value<second_time_t> {
public:
	static bool is_valid(const second_time_t& t) { return !(t == value); }
	static const second_time_t value;
};

template<class T> class time_just_before {
};

template<> class time_just_before<nanosecond_time_t> {
public:
	static nanosecond_time_t get(const nanosecond_time_t& t) {
		if (t.nanosecond)
			return nanosecond_time_t(t.second, t.nanosecond - 1);
		else
			return nanosecond_time_t(t.second - 1, 999999999);
	}
};

template<> class time_just_before<second_time_t> {
public:
	static second_time_t get(const second_time_t& t) {
		return t - 1;
	}
};

nanosecond_time_t 
szb_move_time(const nanosecond_time_t& t, int count, SZARP_PROBE_TYPE probe_type, 
		int custom_length = 0);

nanosecond_time_t 
szb_round_time(nanosecond_time_t t, SZARP_PROBE_TYPE probe_type, int custom_length = 0);

}

#endif
