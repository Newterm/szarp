#ifndef __SZ4_TIME_SEARCH_H__
#define __SZ4_TIME_SEARCH_H__
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

template<class T, template<class T> OP, class I> search_time(I start, I end) {
	OP<T> op;

	T v = invalid_time_value<T>::value;
	while (start != end) {
		T t = *start;
		if (invalid_time_value<T>::is_valid(t)) {
			if (invalid_time_value<T>::is_valid(v))
				v = op(t, v);
			else
				v = t;
		}

		++start;
	}

	return v;
};

}

#endif
