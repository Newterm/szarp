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
#include <sstream>
#include <string>
#include <assert.h>
#include "luadatablock.h"
#include "conversion.h"
#include "szbbase.h"
#include "szbdate.h"
#include "liblog.h"

#ifndef NO_LUA

#include "luacalculate.h"

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

LuaDatablock::LuaDatablock(szb_buffer_t * b, TParam * p, int y, int m): CacheableDatablock(b, p, y, m) {}
	
LuaNativeDatablock::LuaNativeDatablock(szb_buffer_t * b, TParam * p, int y, int m): LuaDatablock(b, p, y, m)
{
#ifdef KDEBUG
	sz_log(DATABLOCK_CREATION_LOG_LEVEL, "D: LuaNativeDatablock::LuaNativeDatablock(%s, %d.%d)", param->GetName(), year, month);
#endif

	m_init_in_progress = false;

	this->AllocateDataMemory();

	this->fixed_probes_count = 0;

	if (CacheableDatablock::LoadFromCache()) {
		Refresh();
		return;
	}

	int av_year, av_month;
	time_t end_date = szb_search_last(buffer, param);

	szb_time2my(end_date, &av_year, &av_month);

	if (year > av_year || (year == av_year && month > av_month))
		NOT_INITIALIZED;

	if (end_date > GetEndTime())
		end_date = GetEndTime();

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

void LuaNativeDatablock::FinishInitialization() {
	if (!IsInitialized())
		return;

	if (m_init_in_progress == false) {
		//block has been load from cache
		if (fixed_probes_count > 0)
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
	this->fixed_probes_count = start_probe;

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
		lua_get_val(lua, buffer, probe2time(i, year, month), PT_MIN10, 0, data[i]);
		bool fixedvalue = Lua::fixed.top();
		Lua::fixed.pop();
		if (buffer->last_err != SZBE_OK) {
			NOT_INITIALIZED;
			break;
		}
		if (fixedvalue && i == this->fixed_probes_count) {
			this->fixed_probes_count++;
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
LuaNativeDatablock::Refresh() {

	if(this->fixed_probes_count == this->max_probes)
		return;

	time_t updatetime = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	if(this->last_update_time == updatetime)
		return;

	this->last_update_time = updatetime;

	time_t end_date = szb_search_last(buffer, param);
	if (end_date > GetEndTime())
		end_date = GetEndTime();

	lua_State* lua = Lua::GetInterpreter();
	int ref = param->GetLuaParamReference();
	if (ref == LUA_NOREF)
		ref = compile_lua_param(lua, param);

	lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);

	int new_probes_count = szb_probeind(end_date) + 1;

	assert(new_probes_count >= this->fixed_probes_count);

	if (this->first_data_probe_index >= fixed_probes_count)
		first_data_probe_index = -1;
	if (this->last_data_probe_index >= fixed_probes_count)
		last_data_probe_index = -1;

	for (int i = this->fixed_probes_count; i < new_probes_count; i++) {

		Lua::fixed.push(true);
		lua_get_val(lua, buffer, probe2time(i, year, month), PT_MIN10, 0, data[i]);
		bool fixedvalue = Lua::fixed.top();
		Lua::fixed.pop();
		if (buffer->last_err != SZBE_OK)
			break;
		if (fixedvalue && i == this->fixed_probes_count) {
			this->fixed_probes_count++;
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
