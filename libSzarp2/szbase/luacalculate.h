#ifndef __LUA_CALCULATE_HPP__
#define __LUA_CALCULATE_HPP__
int compile_lua_param(lua_State *lua, TParam *p);
void lua_get_val(lua_State* lua, szb_buffer_t *buffer, time_t start, SZARP_PROBE_TYPE probe_type, int custom_length, double& result);
#endif
