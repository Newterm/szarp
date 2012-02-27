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

#include "szbase/szbbuf.h"

szb_block_t::szb_block_t(szb_buffer_t* b, TParam* p, time_t st, time_t et) :
		buffer(b), param(p), start_time(st), end_time(et), fixed_probes_count(0), locator(NULL)
{
}

void szb_block_t::MarkAsUsed() {
	locator->Used();
}

int szb_block_t::GetFixedProbesCount() {
	return fixed_probes_count;
}

time_t szb_block_t::GetStartTime() {
	return start_time;
}

time_t szb_block_t::GetEndTime() {
	return end_time;
}

szb_block_t::~szb_block_t() {
	if (locator)
		delete locator;
}
