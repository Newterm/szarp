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
#ifndef __SZ4_PATH_H__
#define __SZ4_PATH_H__

#include "sz4/time.h"

namespace sz4 {

template<class C> bool szbase_path(const std::basic_string<C> &path, unsigned& time);

template<class T, class C> T path_to_date(const std::basic_string<C>& path, bool &sz4);

template<class C, class T> std::basic_string<C> date_to_path(const T& t);

}
#endif
