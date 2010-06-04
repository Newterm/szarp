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
 * realdatablock
 * $Id$
 */

#ifndef __SZBASE_REAL_DATABLOCK_H__
#define __SZBASE_REAL_DATABLOCK_H__

#include <config.h>

#include <time.h>

#include "szbbuf.h"
#include "szbdatablock.h"
#include "szarp_config.h"

class RealDatablock: public szb_datablock_t
{
		bool LoadFromFile();
		int probes_from_file;
		bool isEmpty;
		static bool empty_field_initialized;
		static SZBASE_TYPE * empty_field;
	public:
		virtual void Refresh();
		RealDatablock(szb_buffer_t * b, TParam * p, int y, int m);
		~RealDatablock();
		virtual const SZBASE_TYPE * GetData(bool refresh=true);
};

#endif //__SZBASE_REAL_DATABLOCK_H__
