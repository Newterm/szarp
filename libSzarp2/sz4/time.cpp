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

#include "sz4/time.h"

namespace sz4 {

nanosecond_time_t make_nanosecond_time(uint32_t second, uint32_t nanosecond) {
	nanosecond_time_t v = {second, nanosecond};
	return v;
}

long long operator-(const nanosecond_time_t& t1, const nanosecond_time_t& t2) {
	long long d = t1.second;
	d -= t2.second;
	d *= 1000000000;
	d += t1.nanosecond;
	d -= t2.nanosecond;
	return d;
}

bool operator==(const nanosecond_time_t& t1, const nanosecond_time_t& t2) {
	return t1.second == t2.second && t1.nanosecond == t2.nanosecond;
}

long long operator>(const nanosecond_time_t& t1, const nanosecond_time_t& t2) {
	return t1.second > t2.second || (t1.second == t2.second && t1.nanosecond > t2.nanosecond);
}

}
