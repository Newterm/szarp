#include "conversion.h"
#include "szbbase.h"
#include "szbdate.h"
#include "liblog.h"
#include "szbsearch.h"

#include "luacalculate.h"

void lua_get_val(lua_State* lua, szb_buffer_t *buffer, time_t start, SZARP_PROBE_TYPE probe_type, int custom_length, double& result)
{
	lua_pushvalue(lua, -1);	
	lua_pushnumber(lua, start);
	lua_pushnumber(lua, probe_type);
	lua_pushnumber(lua, custom_length);
	int ret = lua_pcall(lua, 3, 1, 0);
	if (ret == 0) {
		if (lua_isnumber(lua, -1))
			result = lua_tonumber(lua, -1);
		else if (lua_isboolean(lua, -1))
			result = lua_toboolean(lua, -1);
		else
			result = SZB_NODATA;
	} else {
		result = SZB_NODATA;
		buffer->last_err = SZBE_LUA_ERROR;
		buffer->last_err_string = SC::U2S((const unsigned char*)lua_tostring(lua, -1));	
	}
	lua_pop(lua, 1);
}

SZBASE_TYPE szb_lua_get_avg(szb_buffer_t* buffer, TParam *param, time_t start_time, time_t end_time, SZBASE_TYPE *psum, int *pcount, SZARP_PROBE_TYPE probe, bool &fixed) {
	double sum = .0;
	size_t data_count = 0;
	double val;
	time_t first_date;
	time_t last_date;
	lua_State* lua = Lua::GetInterpreter();
	int ref = param->GetLuaParamReference();
	if (ref == LUA_NOREF) {
		ref = lua::compile_lua_param(lua, param);
		if (ref == LUA_REFNIL) {
			buffer->last_err = SZBE_LUA_ERROR;
			buffer->last_err_string = SC::U2S((const unsigned char*)lua_tostring(lua, -1));

			lua_pop(lua, 1);

			goto error;
		}
	}
	if (ref == LUA_REFNIL) {
		buffer->last_err = SZBE_LUA_ERROR;
		buffer->last_err_string = L"Invalid LUA param";
		goto error;
	}
	if (!szb_lua_search_first_last_date(buffer, param, probe, first_date, last_date))
		goto error;
	start_time = std::max(start_time, first_date);
	end_time = std::min(end_time, last_date);
	lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);
	for (size_t i = 0; start_time < end_time; ++i, start_time = szb_move_time(start_time, 1, probe, 0)) {
		if (start_time <= last_date) {
			Lua::fixed.push(true);
			lua_get_val(lua, buffer, start_time, probe, 0, val);
			fixed = fixed && Lua::fixed.top();
			Lua::fixed.pop();
		} else {
			fixed = false;
			val = SZB_NODATA;
		}
		if (IS_SZB_NODATA(val))
			continue;
		val = rint(val * pow10(param->GetPrec())) / pow10(param->GetPrec()); 
		sum += val;
		data_count++;
	}
	double ret;
	if (data_count)
		ret = sum / data_count;
	else {
		ret = SZB_NODATA;
		sum = SZB_NODATA;
	}
	if (psum)
		*psum = sum;
	if (pcount)
		*pcount = data_count;
	lua_pop(lua, 1);
	return ret;
error:
	if (psum)
		*psum = SZB_NODATA;
	if (pcount)
		*pcount = 0;
	return SZB_NODATA;
}

