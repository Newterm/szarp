#ifndef __SZ4_PATH_H__
#define __SZ4_PATH_H__
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

namespace path_impl {

template<class C> struct zero_char_trait { };

template<> struct zero_char_trait<char> {
	static const char zero = '0';
};

template<> struct zero_char_trait<wchar_t> {
	static const wchar_t zero = L'0';
};

template<class T, class C> struct path_to_date_converter {
};


template<class C> struct path_to_date_converter<second_time_t, C> {
	static second_time_t convert(const std::basic_string<C> &path) {
		second_time_t v = 0;

		if (path.size() < 14)
			return invalid_time_value<second_time_t>::value();

		for (size_t i = 0; i < 10; i++) {
			v *= 10;
			v += int(path[path.size() - 14 + i] - zero_char_trait<C>::zero);
		}

		return v;	
	}
};

template<class C> struct path_to_date_converter<nanosecond_time_t, C>  {
	static nanosecond_time_t convert(const std::basic_string<C> &path) {
		nanosecond_time_t v(0, 0);
	
		if (path.size() < 24)
			return invalid_time_value<nanosecond_time_t>::value();
	
		for (size_t i = 0; i < 10; i++) {
			v.nanosecond *= 10;
			v.nanosecond += path[path.size() - 14 + i] - zero_char_trait<C>::zero;
		}
	
		for (size_t i = 0; i < 10; i++) {
			v.second *= 10;
			v.second += path[path.size() - 24 + i] - zero_char_trait<C>::zero;
		}
	
		return v;
	}
};

}

template<class T> T path_to_date(const std::string& path) {
	return path_impl::path_to_date_converter<T, char>::convert(path);
}

template<class T> T path_to_date(const std::wstring& path) {
	return path_impl::path_to_date_converter<T, wchar_t>::convert(path);
}

}
#endif
