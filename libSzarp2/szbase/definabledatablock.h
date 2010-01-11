/* 
  SZARP: SCADA software 

*/
/* 
 *
 * ndefinable - new way of definable calculating
 * $Id$
 */

#ifndef __SZBASE_DEFINABLE_DATABLOCK_H__
#define __SZBASE_DEFINABLE_DATABLOCK_H__

#include <time.h>

#include "szbbuf.h"
#include "cacheabledatablock.h"
#include "szbdatablock.h"
 
time_t
szb_definable_search_data(szb_buffer_t * buffer, TParam * param, 
		time_t start_time, time_t end_time, int direction);

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
		int GetBlocksUsedInFormula(szb_datablock_t ** dblocks, int &fixedprobes);
};


#endif //__SZBASE_DEFINABLE_DATABLOCK_H__

