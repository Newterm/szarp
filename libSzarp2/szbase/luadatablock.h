/* 
  SZARP: SCADA software 

*/
#ifndef __SZBLUA_H__
#define __SZBLUA_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef NO_LUA

#include "lua.hpp"
#include "szbbuf.h"
#include "szbdatablock.h"

#include "cacheabledatablock.h"

#include "szbcancelhandle.h"

class LuaDatablock: public CacheableDatablock {
public:
	LuaDatablock(szb_buffer_t * b, TParam * p, int y, int m);
};

class LuaNativeDatablock: public LuaDatablock
{
	public:
		LuaNativeDatablock(szb_buffer_t * b, TParam * p, int y, int m);
		~LuaNativeDatablock(){this->Cache();};
		virtual void FinishInitialization();
		virtual void Refresh();
	protected:
		int m_probes_to_compute;	//numer of probes to calculate during initialization phase
		bool m_init_in_progress;
	private:
			
};

#endif

#endif
