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
#ifndef __SZ4_BLOCK_FOR_DATE_H__
#define __SZ4_BLOCK_FOR_DATE_H__

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "path.h"
#include "time.h"

#if BOOST_FILESYSTEM_VERSION != 3
#define directory_iterator wdirectory_iterator
#endif

namespace sz4 {

template<class T> std::wstring find_block_for_date(const std::wstring& dir, const T& time) {
	namespace fs = boost::filesystem;

	std::wstring file;
	T found_time(time_trait<T>::invalid_value);
	bool sz4;

	for (fs::directory_iterator i(dir);
			i != fs::directory_iterator();
			i++) {
#if BOOST_FILESYSTEM_VERSION == 3
		T file_time(path_to_date<T>(i->path().string(), sz4));
#else
		T file_time(path_to_date<T>(i->path().file_string(), sz4));
#endif
		if (!time_trait<T>::is_valid(file_time))
			continue;

		if (file_time > time)
			continue;

		if (!time_trait<T>::is_valid(found_time) || (time - file_time < time - found_time)) {
			found_time = file_time;
#if BOOST_FILESYSTEM_VERSION == 3
			file = i->path().wstring();
#else
			file = i->path().file_string();
#endif
		}
	}

	return file;
}

}

#if BOOST_FILESYSTEM_VERSION != 3
#undef directory_iterator
#endif

#endif
