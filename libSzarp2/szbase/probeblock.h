#ifndef PROBEBLOCK_H__
#define PROBEBLOCK_H__

time_t 
szb_real_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle);

time_t 
szb_combined_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle);

time_t 
szb_definable_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle);

time_t 
szb_lua_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle);
#endif
