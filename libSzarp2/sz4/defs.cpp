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

#include "sz4/defs.h"

#include "szarp_config.h"

#include <cmath> 

namespace sz4 {

bool value_is_no_data(const double& v) {
	return std::isnan(v);
}

bool value_is_no_data(const float& v) {
	return isnanf(v);
}

template<> float no_data<float>() {
	return nanf("");
}

template<> double no_data<double>() {
	return nan("");
}

}

