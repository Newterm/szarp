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

const nanosecond_time_t time_trait<nanosecond_time_t>::invalid_value = make_nanosecond_time(-1, -1);
const nanosecond_time_t time_trait<nanosecond_time_t>::first_valid_time = make_nanosecond_time(0, 0);
const nanosecond_time_t time_trait<nanosecond_time_t>::last_valid_time = make_nanosecond_time(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max() - 1);

const second_time_t time_trait<second_time_t>::invalid_value = std::numeric_limits<second_time_t>::max();
const second_time_t time_trait<second_time_t>::first_valid_time = 0;
const second_time_t time_trait<second_time_t>::last_valid_time = std::numeric_limits<second_time_t>::max() - 1;

nanosecond_time_t make_nanosecond_time(uint32_t second, uint32_t nanosecond) {
	return nanosecond_time_t(second, nanosecond);
}

nanosecond_time_t 
szb_move_time(const nanosecond_time_t& t, int count, SZARP_PROBE_TYPE probe_type, 
		int custom_length) {
	switch (probe_type) {
		case PT_MSEC10: {
			unsigned sec = t.second;
			sec += count / 10;

			unsigned nsec =	t.nanosecond + 100000000 * (count % 10);
			unsigned carry = nsec / 1000000000;

			sec += carry;
			nsec -= carry * 1000000000;
			return make_nanosecond_time(sec, nsec);
		}
		case PT_HALFSEC: {
			unsigned sec = t.second;
			sec += count / 2;

			unsigned nsec =	t.nanosecond + 500000000 * (count % 2);
			unsigned carry = nsec / 1000000000;

			sec += carry;
			nsec -= carry * 1000000000;
			return make_nanosecond_time(sec, nsec);
		}
		default:
			return make_nanosecond_time(szb_move_time(time_t(t.second), count, probe_type, custom_length), t.nanosecond);
	}
}

nanosecond_time_t 
szb_round_time(nanosecond_time_t t, SZARP_PROBE_TYPE probe_type, int custom_length) {
	switch (probe_type) {
		case PT_HALFSEC:
			return make_nanosecond_time(t.second, t.nanosecond / 500000000 * 500000000);
		default:
			return make_nanosecond_time(szb_round_time(time_t(t.second), probe_type, custom_length), 0);
	}
}

nanosecond_time_t::nanosecond_time_t(const second_time_t& time) {
	if (time_trait<second_time_t>::is_valid(time)) {
		second = time;
		nanosecond = 0;
	} else {
		*this = time_trait<nanosecond_time_t>::invalid_value;
	}	
}

nanosecond_time_t::operator second_time_t() const {
	return sz4::time_trait<nanosecond_time_t>::is_valid(*this)
		?
		second 
		:
		time_trait<second_time_t>::invalid_value;
}

std::ostream& operator<<(std::ostream& s, const nanosecond_time_t &t) {
	return s << "(" << t.second << "," << t.nanosecond << ")";
}

namespace detail {

	// A utility class to reset the format flags for an istream at end
	// of scope, even in case of exceptions
	// Taken verbatim from boost.
	struct resetter {
		resetter(std::istream& is) : is_(is), f_(is.flags()) {}
		~resetter() { is_.flags(f_); }
		std::istream& is_;
		std::istream::fmtflags f_;      // old GNU c++ lib has no ios_base
	};

}

std::istream& operator>>(std::istream& s, nanosecond_time_t &t) {
	char c;

	detail::resetter sentry(s);

	c = s.get();
	if (c != '(')
		s.clear(std::istream::badbit);

	s >> std::noskipws;

	decltype(t.second) second;
	s >> second;
	t.second = second;

	c = s.get();
	if (c != ',')
		s.clear(std::istream::badbit);

	decltype(t.nanosecond) nanosecond;
	s >> nanosecond;
	t.nanosecond = nanosecond;

	c = s.get();
	if (c != ')')
		s.clear(std::istream::badbit);

	return s;
}


}
