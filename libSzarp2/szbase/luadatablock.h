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

void szb_lua_get_values(szb_buffer_t* buffer, TParam *param, time_t start_time, time_t end_time, SZARP_PROBE_TYPE p, SZBASE_TYPE *ret);

SZBASE_TYPE szb_lua_get_avg(szb_buffer_t *buffer, TParam *param, time_t start_time, time_t end_time, SZBASE_TYPE *psum, int *pcount, SZARP_PROBE_TYPE probe, bool &fixed);

time_t szb_lua_search_data(szb_buffer_t * buffer, TParam * param , time_t start, time_t end, int direction, SZARP_PROBE_TYPE probe, SzbCancelHandle * c_handle = NULL);

SZBASE_TYPE szb_lua_calc_avg(szb_buffer_t * buffer, TParam * param, time_t time, SZARP_PROBE_TYPE probe_type, int custom_length);

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
