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
/* 
 *
 * combineddatablock
 * $Id$
 */

#ifndef __SZBASE_COMBINED_DATABLOCK_H__
#define __SZBASE_COMBINED_DATABLOCK_H__

#include <time.h>
#include <string>
#include "szbbuf.h"
#include "szbdatablock.h"
 
time_t
szb_combined_search_data(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction);
int
szb_load_data(szb_buffer_t * buffer, const std::wstring& path, SZB_FILE_TYPE * pdata_buffer,
	int start, int count);

class CombinedDatablock: public szb_datablock_t
{
	public:
		CombinedDatablock(szb_buffer_t * b, TParam * p, int y, int m);
		virtual void Refresh();
	protected:
		
};

#endif //__SZBASE_COMBINED_DATABLOCK_H__

