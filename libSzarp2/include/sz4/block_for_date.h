#ifndef __SZ4_BLOCK_FOR_DATE_H__
#define __SZ4_BLOCK_FOR_DATE_H__
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

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "path.h"
#include "time.h"

namespace sz4 {

template<class T> std::wstring find_block_for_date(const std::wstring& dir, const T& time) {
	namespace fs = boost::filesystem;

	std::wstring file;
	T found_time(invalid_time_value<T>::value());

	for (fs::wdirectory_iterator i(dir); 
			i != fs::wdirectory_iterator(); 
			i++) {
		
		T file_time(path_to_date<T>(i->path().file_string()));
		if (!invalid_time_value<T>::is_valid(file_time))
			continue;

		if (file_time > time)
			continue;

		if (!invalid_time_value<T>::is_valid(found_time) || (time - file_time < time - found_time)) {
			found_time = file_time;
			file = i->path().file_string();
		}
	}

	return file;
}

}

#endif
