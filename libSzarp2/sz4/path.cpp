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

template<class C> struct char_trait { };

template<> struct char_trait<char> {
	static const char zero = '0';
	static const char s = 's';
	static const char z = 'z';
	static const char b = 'b';
	static const char _4 = '4';
	static const char dot = '.';
};

template<> struct char_trait<wchar_t> {
	static const wchar_t zero = L'0';
	static const wchar_t s = L's';
	static const wchar_t z = L'z';
	static const wchar_t b = L'b';
	static const wchar_t _4 = '4';
	static const wchar_t dot = L'.';
};

template<class T, class C> struct path_to_date_converter {
	static T convert(const std::basic_string<C> &path, bool& sz4);
};

template<class C> struct path_to_date_converter<second_time_t, C> {

	static second_time_t convert(const std::basic_string<C> &path, bool& sz4) {
		second_time_t v = 0;

		sz4 = !szbase_path(path, v);
		if (sz4 == false)
			return v;

		size_t path_size = path.size();
		if (path_size < 14)
			return time_trait<second_time_t>::invalid_value;

		typedef char_trait<C> T;

		if (path[path_size - 4] != T::dot
				|| path[path_size - 3] != T::s
				|| path[path_size - 2] != T::z
				|| path[path_size - 1] != T::_4)
			return time_trait<second_time_t>::invalid_value;

		for (size_t i = 0; i < 10; i++) {
			v *= 10;
			v += int(path[path_size - 14 + i] - char_trait<C>::zero);
		}

		return v;	
	}

};

template<class C> struct path_to_date_converter<nanosecond_time_t, C> {

	static nanosecond_time_t convert(const std::basic_string<C> &path, bool& sz4) {
		sz4 = false;

		nanosecond_time_t v(0, 0);

		size_t path_size = path.size();
		if (path_size < 24)
			return time_trait<nanosecond_time_t>::invalid_value;

		typedef char_trait<C> T;
		if (path[path_size - 4] != T::dot
				|| path[path_size - 3] != T::s
				|| path[path_size - 2] != T::z
				|| path[path_size - 1] != T::_4)
			return time_trait<nanosecond_time_t>::invalid_value;

		for (size_t i = 0; i < 10; i++) {
			v.nanosecond *= 10;
			v.nanosecond += path[path.size() - 14 + i] - char_trait<C>::zero;
		}

		for (size_t i = 0; i < 10; i++) {
			v.second *= 10;
			v.second += path[path.size() - 24 + i] - char_trait<C>::zero;
		}

		sz4 = true;
		return v;
	}
};

template struct path_to_date_converter<nanosecond_time_t, char>;
template struct path_to_date_converter<second_time_t, char>;
template struct path_to_date_converter<nanosecond_time_t, wchar_t>;
template struct path_to_date_converter<second_time_t, wchar_t>;

}

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


template<class T, class C> T path_to_date(const std::basic_string<C>& path, bool &sz4) {
	return path_impl::path_to_date_converter<T, C>::convert(path, sz4);
}

template nanosecond_time_t path_to_date(const std::wstring& path, bool &sz4);
template second_time_t path_to_date(const std::wstring& path, bool &sz4);

template nanosecond_time_t path_to_date(const std::string& path, bool &sz4);
template second_time_t path_to_date(const std::string& path, bool &sz4);

}
