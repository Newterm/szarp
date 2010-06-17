#ifndef __LUA_CALCULATE_HPP__
#define __LUA_CALCULATE_HPP__
int compile_lua_param(lua_State *lua, TParam *p);

void lua_get_val(lua_State* lua, szb_buffer_t *buffer, time_t start, SZARP_PROBE_TYPE probe_type, int custom_length, double& result);

SZBASE_TYPE szb_lua_get_avg(szb_buffer_t *buffer, TParam *param, time_t start_time, time_t end_time, SZBASE_TYPE *psum, int *pcount, SZARP_PROBE_TYPE probe, bool &fixed);

#endif
