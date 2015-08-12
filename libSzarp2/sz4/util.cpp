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

#include "sz4/util.h"

namespace sz4 {

SZARP_PROBE_TYPE get_probe_type_step(SZARP_PROBE_TYPE pt) {
	switch (pt) {
		case PT_HALFSEC:
		case PT_SEC10:
		case PT_SEC:
		case PT_MIN10:
			return pt;
		default:
			return PT_MIN10;
	}
}

}

