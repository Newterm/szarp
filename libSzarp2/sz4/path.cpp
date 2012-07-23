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

#include <string>
#include "sz4/path.h"

namespace sz4 {

template<> second_time_t path_to_date<second_time_t>(const std::wstring& path) {
	second_time_t v = 0;

	if (path.size() < 14)
		return invalid_time_value<second_time_t>::value();

	for (size_t i = 0; i < 10; i++) {
		v *= 10;
		v += int(path[path.size() - 14 + i] - L'0');
	}

	return v;	
}

template<> nanosecond_time_t path_to_date<nanosecond_time_t>(const std::wstring& path) {
	nanosecond_time_t v = {0, 0};

	if (path.size() < 24)
		return invalid_time_value<nanosecond_time_t>::value();

	for (size_t i = 0; i < 10; i++) {
		v.nanosecond *= 10;
		v.nanosecond += path[path.size() - 14 + i] - L'0';
	}

	for (size_t i = 0; i < 10; i++) {
		v.second *= 10;
		v.second += path[path.size() - 24 + i] - L'0';
	}

	return v;
}


}
