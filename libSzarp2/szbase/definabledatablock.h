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
#ifndef __SZBASE_DEFINABLE_DATABLOCK_H__
#define __SZBASE_DEFINABLE_DATABLOCK_H__

#include "szbbuf.h"
#include "cacheabledatablock.h"
#include "szbdatablock.h"
 
class DefinableDatablock: public CacheableDatablock
{
	public:
		DefinableDatablock(szb_buffer_t * b, TParam * p, int y, int m);
		~DefinableDatablock(){this->Cache();};
		virtual void Refresh();
	protected:
		virtual bool IsCachable();	
	private:
		bool TestParam(TParam *param, int year, int month, time_t t);
		int GetBlocksUsedInFormula(const double** dblocks, TParam** params, int &fixedprobes);
};


#endif //__SZBASE_DEFINABLE_DATABLOCK_H__

