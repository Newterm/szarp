#include "conversion.h"
#include "szbbase.h"
#include "szbdate.h"
#include "liblog.h"

#include "luacalculate.h"

void lua_get_val(lua_State* lua, szb_buffer_t *buffer, time_t start, SZARP_PROBE_TYPE probe_type, int custom_length, double& result)
{
	lua_pushvalue(lua, -1);	
	lua_pushnumber(lua, start);
	lua_pushnumber(lua, probe_type);
	lua_pushnumber(lua, custom_length);
	int _ret = lua_pcall(lua, 3, 1, 0);
	if (_ret != 0) {
		result = SZB_NODATA;
		buffer->last_err = SZBE_LUA_ERROR;
		buffer->last_err_string = SC::U2S((const unsigned char*)lua_tostring(lua, -1));	
	} else 
		result = lua_tonumber(lua, -1);
	lua_pop(lua, 1);
}

int compile_lua_param(lua_State *lua, TParam *p) {
	int lua_function_reference = LUA_NOREF;
	if (szb_compile_lua_formula(lua, (const char*) p->GetLuaScript(), (const char*)SC::S2U(p->GetName()).c_str()))
		lua_function_reference = luaL_ref(lua, LUA_REGISTRYINDEX);
	else {
		sz_log(0, "Error compiling param %ls: %s\n", p->GetName().c_str(), lua_tostring(lua, -1));
		lua_function_reference = LUA_REFNIL;
	}
	p->SetLuaParamRef(lua_function_reference);
	return lua_function_reference;

}

