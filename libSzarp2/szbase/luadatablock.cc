/* 
  SZARP: SCADA software 

*/

#include <sstream>
#include <string>
#include <assert.h>
#include "luadatablock.h"
#include "conversion.h"
#include "szbbase.h"
#include "szbdate.h"
#include "liblog.h"

#ifndef NO_LUA

using std::ostringstream;
using std::string;
using std::endl;

#if 0
static void lua_printstack(const char* a, lua_State *lua) {
	int n = lua_gettop(lua); //number of arguments
	fprintf(stderr, "%s stack size %d\n", a, n);
	for (int i = 1; i <= n; i++)
	{
		int type = lua_type(lua, i);
		fprintf(stderr, "Stack (%d) val: %s\n", i , lua_typename(lua, type));
	}

}
#endif

static int compile_lua_param(lua_State *lua, TParam *p) {
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

void LUA_GET_VAL(lua_State* lua, szb_buffer_t *BUFFER, time_t START, SZARP_PROBE_TYPE PROBE_TYPE, int CUSTOM_LENGTH, double& RESULT) 
{													
	lua_pushvalue(lua, -1);		
	lua_pushnumber(lua, (START));									
	lua_pushnumber(lua, (PROBE_TYPE));								
	lua_pushnumber(lua, (CUSTOM_LENGTH));								
	int _ret = lua_pcall(lua, 3, 1, 0);								
	if (_ret != 0) {										
		RESULT = SZB_NODATA;									
		BUFFER->last_err = SZBE_LUA_ERROR;							
		BUFFER->last_err_string = SC::U2S((const unsigned char*)lua_tostring(lua, -1));		
	} else 												
		RESULT = lua_tonumber(lua, -1);								
	lua_pop(lua, 1);										
}

void szb_lua_get_values(szb_buffer_t *buffer, TParam *param, time_t start_time, time_t end_time, SZARP_PROBE_TYPE probe_type, SZBASE_TYPE *ret) {
	lua_State* lua = Lua::GetInterpreter();
	int ref = param->GetLuaParamReference();
	if (ref == LUA_NOREF) {
		ref = compile_lua_param(lua, param);
		if (ref == LUA_REFNIL) {
			sz_log(0, "Error compiling param %ls: %ls\n", param->GetName().c_str(), SC::U2S((const unsigned char*)lua_tostring(lua, -1)).c_str());

			buffer->last_err = SZBE_LUA_ERROR;
			buffer->last_err_string = SC::U2S((const unsigned char*)lua_tostring(lua, -1));

			lua_pop(lua, 1);
		}
		goto error;
	}

	if (ref == LUA_REFNIL) {
		buffer->last_err = SZBE_LUA_ERROR;
		buffer->last_err_string = L"Invalid LUA param";

		goto error;
	}

	lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);

	for (size_t i = 0; start_time < end_time; ++i, start_time = szb_move_time(start_time, 1, probe_type, 0)) {
		Lua::fixed.push(true);
		LUA_GET_VAL(lua, buffer, start_time, probe_type, 0, ret[i]);
		if (buffer->last_err != SZBE_OK)
			break;
		ret[i] = rint(ret[i] * pow10(param->GetPrec())) / pow10(param->GetPrec()); 
		Lua::fixed.pop();
	}
	lua_pop(lua, 1);

	return;

error:
	for (size_t i = 0; start_time < end_time; ++i, start_time = szb_move_time(start_time, 1, probe_type, 0)) 
		ret[i] = SZB_NODATA;


}

time_t szb_lua_search_data(szb_buffer_t * buffer, TParam * param ,
		time_t start, time_t end, int direction, SZARP_PROBE_TYPE probe, SzbCancelHandle * c_handle) {
#ifdef KDEBUG
	sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_lua_search_data: %s, s: %ld, e: %ld, d: %d",
	    param->GetName(), start, end, direction);
#endif
	if (param->GetFormulaType() == TParam::LUA_VA)
		probe = PT_MIN10;

	start = szb_round_time(start, probe, 0);

	time_t param_first_av_date;

	double val = SZB_NODATA;

	if (param->GetLuaStartDateTime() > 0) {
		param_first_av_date = param->GetLuaStartDateTime();
	} else {
		param_first_av_date = buffer->first_av_date;
	}
	param_first_av_date += param->GetLuaStartOffset();
	param_first_av_date = szb_round_time(param_first_av_date, probe, 0);

	time_t param_last_av_date = szb_search_last(buffer, param);

	if (direction == 0) {
		if (start < param_first_av_date || start > param_last_av_date)
			return -1;

		val = szb_get_probe(buffer, param, start, probe, 0);

		time_t ret = IS_SZB_NODATA(val) ? -1 : start;
		sz_log(SEARCH_DATA_LOG_LEVEL, "Search data called with direction = %d, start = %d, end %d, result = %d, found value = %f", direction, (int)start, (int)end, (int)ret, val);
		return ret;
	}

	if (direction > 0 && end == -1)
		end = param_last_av_date;

	if (direction < 0 && end == -1)
		end = param_first_av_date;

	if (direction > 0) {
		if (start > param_last_av_date || end < param_first_av_date)
			return -1;
		if (start < param_first_av_date)
			start = param_first_av_date;
		if (end > param_last_av_date)
			end = param_last_av_date;
	}
	else {
		if (start < param_first_av_date || end > param_last_av_date)
			return -1;
		if (start > param_last_av_date)
			start = param_last_av_date;
		if (end < param_first_av_date)
			end = param_first_av_date;
	}

	end = szb_round_time(end, probe, 0);

#define INTERVAL 500
#define BREAK_IF_CANCELED \
	if(c_handle) { \
		counter--; \
		if (counter == 0) { \
			counter = INTERVAL; \
			if(c_handle->IsStopFlag()) { \
				sz_log(SEARCH_DATA_LOG_LEVEL, "Search data called with direction = %d, start = %d, end %d, stopped by cancel handle", direction, (int)start, (int)end); \
				return -1; \
			} \
		} \
	}

	int counter = INTERVAL;
	if (c_handle) {
		c_handle->Start();
		counter = INTERVAL;
	}

	if (direction > 0) {
		for (;start <= end; start = szb_move_time(start, 1, probe, 0)) {
			val = szb_get_probe(buffer, param, start, probe, 0);
			if (!IS_SZB_NODATA(val) || buffer->last_err != SZBE_OK)
				break;
			BREAK_IF_CANCELED;
		}
	}
	else {
		for (;start >= end; start = szb_move_time(start, -1, probe, 0)) {
			val = szb_get_probe(buffer, param, start, probe, 0);
			if (!IS_SZB_NODATA(val) || buffer->last_err != SZBE_OK)
				break;
			BREAK_IF_CANCELED;
		}
	}

#undef INTERVAL
#undef BREAK_IF_CANCELED

	time_t ret;
	if (IS_SZB_NODATA(val))
		ret = -1;
	else
		ret = start;

	sz_log(SEARCH_DATA_LOG_LEVEL, "Search data called with direction = %d, start = %d, end %d, result = %d, found value = %f", direction, (int)start, (int)end, (int)ret, val);

	return ret;

}


SZBASE_TYPE szb_lua_get_avg(szb_buffer_t* buffer, TParam *param, time_t start_time, time_t end_time, SZBASE_TYPE *psum, int *pcount, SZARP_PROBE_TYPE probe, bool &fixed) {
	double sum = .0;
	size_t data_count = 0;
	double val;
	time_t param_first_av_date;
	time_t param_last_av_date;

	lua_State* lua = Lua::GetInterpreter();

	int ref = param->GetLuaParamReference();
	if (ref == LUA_NOREF) {
		ref = compile_lua_param(lua, param);

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

	if (param->GetFormulaType() == TParam::LUA_VA)
		probe = PT_MIN10;

	lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);

	if (param->GetLuaStartDateTime() > 0) {
		param_first_av_date = param->GetLuaStartDateTime();
	} else {
		param_first_av_date = buffer->first_av_date;
	}

	param_first_av_date += param->GetLuaStartOffset();
	param_first_av_date = szb_round_time(param_first_av_date, probe, 0);

	param_last_av_date = szb_search_last(buffer, param);

	end_time = std::min(end_time, param_last_av_date);
	start_time = std::max(start_time, param_first_av_date);

	for (size_t i = 0; start_time < end_time; ++i, start_time = szb_move_time(start_time, 1, probe, 0)) {
		if (start_time <= param_last_av_date) {
			Lua::fixed.push(true);
			LUA_GET_VAL(lua, buffer, start_time, probe, 0, val);
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
	
LuaDatablock::LuaDatablock(szb_buffer_t * b, TParam * p, int y, int m): CacheableDatablock(b, p, y, m)
{
#ifdef KDEBUG
	sz_log(DATABLOCK_CREATION_LOG_LEVEL, "D: LuaDatablock::LuaDatablock(%s, %d.%d)", param->GetName(), year, month);
#endif

	m_init_in_progress = false;

	this->AllocateDataMemory();

	this->first_non_fixed_probe = 0;

	if (CacheableDatablock::LoadFromCache()) {
		Refresh();
		return;
	}

	int av_year, av_month;
	time_t end_date = szb_search_last(buffer, param);

	szb_time2my(end_date, &av_year, &av_month);

	if (year > av_year || (year == av_year && month > av_month))
		NOT_INITIALIZED;

	if (end_date > GetBlockLastDate())
		end_date = GetBlockLastDate();

	m_probes_to_compute = szb_probeind(end_date) + 1;

	for(int i = m_probes_to_compute; i < max_probes; i++)
		data[i] = SZB_NODATA;

	this->last_update_time = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	lua_State* lua = Lua::GetInterpreter();
	int ref = param->GetLuaParamReference();
	if (ref == LUA_NOREF)
		ref = compile_lua_param(lua, param);

	if (ref == LUA_REFNIL)
		NOT_INITIALIZED;
}

void LuaDatablock::FinishInitialization() {
	if (!IsInitialized())
		return;

	if (m_init_in_progress == false) {
		//block has been load from cache
		if (first_non_fixed_probe > 0)
			return;
		m_init_in_progress = true;
	} else
		return;

	time_t start_date;
	if (param->GetLuaStartDateTime() > 0) {
		start_date = param->GetLuaStartDateTime();
	} else {
		start_date = buffer->first_av_date;
	}
	start_date += param->GetLuaStartOffset();

	int year_start, month_start;
	szb_time2my(start_date, &year_start, &month_start);

	int start_probe = 0;
	if (year_start > year || (year_start == year && month_start > month))
		start_probe = m_probes_to_compute;
	else if (year_start == year && month == month_start)
		start_probe = szb_probeind(start_date);
	else
		start_probe = 0;

	for (int i = 0; i < start_probe; i++)
		this->data[i] = SZB_NODATA;
	this->first_non_fixed_probe = start_probe;

	time_t end_date = buffer->GetMeanerDate() + param->GetLuaEndOffset();

	int year_end, month_end;
	szb_time2my(end_date, &year_end, &month_end);

	int end_probe;
	if (year_end < year || 
			(year_end == year && month_end < month))
		end_probe = 0;
	else if (year_end == year && month_end == month)
		end_probe = szb_probeind(end_date) + 1;
	else
		end_probe = m_probes_to_compute;
	

	int ref = param->GetLuaParamReference();
	lua_State* lua = Lua::GetInterpreter();
	lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);
	for (int i = start_probe; i < end_probe; i++) {
		Lua::fixed.push(true);
		LUA_GET_VAL(lua, buffer, probe2time(i, year, month), PT_MIN10, 0, data[i]);
		bool fixedvalue = Lua::fixed.top();
		Lua::fixed.pop();
		if (buffer->last_err != SZBE_OK) {
			NOT_INITIALIZED;
			break;
		}
		if (fixedvalue && i == this->first_non_fixed_probe) {
			this->first_non_fixed_probe++;
		}
		if (!IS_SZB_NODATA(this->data[i])) {
			data[i] = rint(data[i] * pow10(param->GetPrec())) / pow10(param->GetPrec()); 
			if (this->first_data_probe_index < 0)
				this->first_data_probe_index = i;
			this->last_data_probe_index = i;
		}
	}
	lua_pop(lua, 1);

	for (int i = end_probe; i < m_probes_to_compute; i++)
		data[i] = SZB_NODATA;

	m_init_in_progress = false;
}

void
LuaDatablock::Refresh() {

	if(this->first_non_fixed_probe == this->max_probes)
		return;

	time_t updatetime = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	if(this->last_update_time == updatetime)
		return;

	this->last_update_time = updatetime;

	time_t end_date = szb_search_last(buffer, param);
	if (end_date > GetBlockLastDate())
		end_date = GetBlockLastDate();

	lua_State* lua = Lua::GetInterpreter();
	int ref = param->GetLuaParamReference();
	if (ref == LUA_NOREF)
		ref = compile_lua_param(lua, param);

	lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);

	int new_probes_count = szb_probeind(end_date) + 1;

	assert(new_probes_count >= this->first_non_fixed_probe);

	if (this->first_data_probe_index >= first_non_fixed_probe)
		first_data_probe_index = -1;
	if (this->last_data_probe_index >= first_non_fixed_probe)
		last_data_probe_index = -1;

	for (int i = this->first_non_fixed_probe; i < new_probes_count; i++) {

		Lua::fixed.push(true);
		LUA_GET_VAL(lua, buffer, probe2time(i, year, month), PT_MIN10, 0, data[i]);
		bool fixedvalue = Lua::fixed.top();
		Lua::fixed.pop();
		if (buffer->last_err != SZBE_OK)
			break;
		if (fixedvalue && i == this->first_non_fixed_probe) {
			this->first_non_fixed_probe++;
		}
		if(!IS_SZB_NODATA(this->data[i])) {
			data[i] = rint(data[i] * pow10(param->GetPrec())) / pow10(param->GetPrec()); 
			if(this->first_data_probe_index < 0)
				this->first_data_probe_index = i;
			this->last_data_probe_index = i;
		}

	}
	lua_pop(lua, 1);

	return;
}

#endif
