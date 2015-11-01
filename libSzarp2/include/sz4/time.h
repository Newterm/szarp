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
#include <iostream>
#include <limits>

#include "szarp_base_common/time.h"

namespace sz4 {

typedef uint32_t second_time_t;

struct nanosecond_time_t {
	nanosecond_time_t() : second(0), nanosecond(0) {}
	nanosecond_time_t(uint32_t _second, uint32_t _nanosecond) : second(_second), nanosecond(_nanosecond) {}
	nanosecond_time_t(double _time) : second(_time), nanosecond((_time - (long)_time) * 1000000000) {}
	nanosecond_time_t(const second_time_t& time);
	bool operator==(const nanosecond_time_t& t) const { return second == t.second && nanosecond == t.nanosecond; }
	long long operator- (const nanosecond_time_t& t) const {
		long long d = second;
		d -= t.second;
		d *= 1000000000;
		d += nanosecond;
		d -= t.nanosecond;
		return d;
	}
	nanosecond_time_t& operator+=(long long i) {
		second += i / 1000000000;
		nanosecond += i % 1000000000;

		if (nanosecond > 1000000000) {
			second += 1;
			nanosecond -= 1000000000;
		}
		return *this;
	}
	bool operator< (const nanosecond_time_t& t) const { return second < t.second || (second == t.second && nanosecond < t.nanosecond); } 
	bool operator<= (const nanosecond_time_t& t) const { return second < t.second || (second == t.second && nanosecond <= t.nanosecond); } 
	bool operator> (const nanosecond_time_t& t) const { return second > t.second || (second == t.second && nanosecond > t.nanosecond); } 
	bool operator>= (const nanosecond_time_t& t) const { return second > t.second || (second == t.second && nanosecond >= t.nanosecond); } 
	operator second_time_t() const;
	operator double() const { return second + double(nanosecond) / 1000000000; }
	uint32_t second;
	uint32_t nanosecond;
} __attribute__ ((packed));

nanosecond_time_t make_nanosecond_time(uint32_t second, uint32_t nanosecond);

template<class FT, class TT> class round_up {
};

template<class TT> class round_up<TT, TT> {
public:
	TT operator()(const TT& t) const { return t; }
};

template<> class round_up<sz4::nanosecond_time_t, sz4::second_time_t> {
public:
	sz4::second_time_t operator()(const sz4::nanosecond_time_t& t) const { 
		if (t.nanosecond)
			if (t.second < std::numeric_limits<sz4::second_time_t>::max())
				return t.second + 1;
			else
				return t.second;
		else
			return t.second;
	}
};

template<> class round_up<sz4::second_time_t, sz4::nanosecond_time_t> {
public:
	sz4::nanosecond_time_t operator()(const sz4::second_time_t& t) const { 
		return sz4::nanosecond_time_t(t, 0);
	}
};

template<class FT, class TT> struct probe_adapter {
	void operator()(SZARP_PROBE_TYPE &probe) {};
};

template<> struct probe_adapter<nanosecond_time_t, second_time_t> {
	void operator()(SZARP_PROBE_TYPE &probe) {
		switch (probe) {
			case PT_MSEC10:
			case PT_HALFSEC:
				probe = PT_SEC;
				break;
			case PT_SEC:
			case PT_SEC10:
			case PT_MIN10:
			case PT_HOUR:
			case PT_HOUR8:
			case PT_DAY:
			case PT_WEEK:
			case PT_MONTH:
			case PT_YEAR:
			case PT_CUSTOM:
				break;
		}
	};
};

template<class T> class time_trait { };

template<> class time_trait<nanosecond_time_t> {
public:
	static bool is_valid(const nanosecond_time_t& t) { return !(t == invalid_value); }

	static const nanosecond_time_t invalid_value;

	static const nanosecond_time_t first_valid_time;
	static const nanosecond_time_t last_valid_time;
};

template<> class time_trait<second_time_t> {
public:
	static bool is_valid(const second_time_t& t) { return !(t == invalid_value); }

	static const second_time_t invalid_value;

	static const second_time_t first_valid_time;
	static const second_time_t last_valid_time;
};

template<class T> class time_just_before {
public:
	static T get(const T& t) {
		return t - 1;
	}
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

template<class T> class time_just_after {
public:
	static T get(const T& t) {
		return t + 1;
	}
};

template<> class time_just_after<nanosecond_time_t> {
public:
	static nanosecond_time_t get(const nanosecond_time_t& t) {
		if (t.nanosecond + 1 == 1000000000)
			return nanosecond_time_t(t.second + 1, 0);
		else
			return nanosecond_time_t(t.second, t.nanosecond + 1);
	}
};

nanosecond_time_t 
szb_move_time(const nanosecond_time_t& t, int count, SZARP_PROBE_TYPE probe_type, 
		int custom_length = 0);

nanosecond_time_t 
szb_round_time(nanosecond_time_t t, SZARP_PROBE_TYPE probe_type, int custom_length = 0);

std::ostream& operator<<(std::ostream& s, const nanosecond_time_t &t);

}

#endif
