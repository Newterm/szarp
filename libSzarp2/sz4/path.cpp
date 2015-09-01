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
#include "szbase/szbdate.h"

namespace sz4 {

namespace path_impl {

template<class C> bool szbase_path(const std::basic_string<C> &path, unsigned& time) {
	if (path.size() < 10)
		return false;

	typedef path_impl::char_trait<C> T;
	size_t path_len = path.size();

	if (path[path_len - 4] != T::dot
			|| path[path_len - 3] != T::s
			|| path[path_len - 2] != T::z
			|| path[path_len - 1] != T::b)
		return false;

	const C _0 = T::zero;
	int year = (path[path_len - 10] - _0) * 1000
			+ (path[path_len - 9]- _0) * 100
			+ (path[path_len - 8]- _0) * 10
			+ (path[path_len - 7]- _0);
	int month = (path[path_len - 6] - _0) * 10
			+ path[path_len - 5] - _0;

	time = probe2time(0, year, month);
	return true;
}

template bool szbase_path(const std::basic_string<char> &path, unsigned& time);
template bool szbase_path(const std::basic_string<wchar_t> &path, unsigned& time);

}

}
