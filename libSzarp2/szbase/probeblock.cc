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

#include "config.h"

#include <boost/asio.hpp>


#include "liblog.h"
#include "szbbuf.h"
#include "szbdefines.h"
#include "szbdate.h"
#include "luacalculate.h"
#include "loptcalculate.h"
#include "proberconnection.h"
#include "szbbase.h"
#include "conversion.h"
#include "szarp_base_common/lua_utils.h"
#include "szb_definable_calculate.h"

static const size_t DEFINABLE_STACK_SIZE  = 200;

szb_probeblock_t::szb_probeblock_t(szb_buffer_t *buffer, TParam* param, time_t start_time, time_t end_time) :
	szb_block_t(buffer, param, start_time, end_time), last_query_id(-1) {
	data = new double[probes_per_block];
	for (int i = 0; i < probes_per_block; i++)
		data[i] = SZB_NODATA;
}

SZB_BLOCK_TYPE szb_probeblock_t::GetBlockType() {
	return SEC10_BLOCK;
}

const SZBASE_TYPE* szb_probeblock_t::GetData(bool refresh) {
	if (refresh)
		Refresh();
	return data;
}

void szb_probeblock_t::Refresh() {
	if (fixed_probes_count == probes_per_block)
		return;
	if (buffer->szbase->GetQueryId() == last_query_id)
		return;
	if (buffer->PrepareConnection() == false)	
		return;
	FetchProbes();
	if (buffer->prober_connection->IsError())
		buffer->prober_connection->ClearError();
	last_query_id = buffer->szbase->GetQueryId();
}

szb_probeblock_t::~szb_probeblock_t() {
	delete [] data;	
}


class szb_probeblock_real_t  : public szb_probeblock_t {
protected:
	virtual void FetchProbes(); 
public:
	szb_probeblock_real_t(szb_buffer_t *buffer, TParam* param, time_t start_time) : szb_probeblock_t(buffer, param, start_time, start_time + (probes_per_block - 1) * SZBASE_PROBE_SPAN) {}
};

class szb_probeblock_combined_t : public szb_probeblock_t {

protected:
	virtual void FetchProbes(); 
public:
	szb_probeblock_combined_t(szb_buffer_t *buffer, TParam* param, time_t start_time) : szb_probeblock_t(buffer, param, start_time, start_time + (probes_per_block - 1) * SZBASE_PROBE_SPAN) {}
};

class szb_probeblock_definable_t : public szb_probeblock_t {

protected:
	virtual void FetchProbes(); 
public:
	szb_probeblock_definable_t(szb_buffer_t *buffer, TParam* param, time_t start_time) : szb_probeblock_t(buffer, param, start_time, start_time + (probes_per_block - 1) * SZBASE_PROBE_SPAN) {}
};

class szb_probeblock_lua_t : public szb_probeblock_t {

protected:
	virtual void FetchProbes(); 
public:
	szb_probeblock_lua_t(szb_buffer_t *buffer, TParam* param, time_t start_time) : szb_probeblock_t(buffer, param, start_time, start_time + (probes_per_block - 1) * SZBASE_PROBE_SPAN) {}
};

class szb_probeblock_lua_opt_t : public szb_probeblock_t {

protected:
	virtual void FetchProbes(); 
public:
	szb_probeblock_lua_opt_t(szb_buffer_t *buffer, TParam* param, time_t start_time) : szb_probeblock_t(buffer, param, start_time, start_time + (probes_per_block - 1) * SZBASE_PROBE_SPAN) {}
};


void szb_probeblock_real_t::FetchProbes() {
	sz_log(10, "Fetcheing real probes");
	int count = buffer->prober_connection->Get(start_time + SZBASE_PROBE_SPAN * fixed_probes_count,
			end_time,
			param->GetSzbaseName());
	if (!count)
		return;

	short values[count];
	buffer->prober_connection->GetData(values, count);
	count = std::min(count, probes_per_block - fixed_probes_count);
 
	int prec10 = pow10(param->GetPrec());
	for (int i = 0; i < count; i++) {
		if (values[i] == SZB_FILE_NODATA)
			data[i + fixed_probes_count] = SZB_NODATA;
		else
			data[i + fixed_probes_count] = double(values[i]) / prec10;
	}
	
	time_t server_time = buffer->prober_connection->GetServerTime();
	if (server_time < start_time)
		return;

	fixed_probes_count = (server_time - start_time) / SZBASE_PROBE_SPAN + 1;
	if (fixed_probes_count > probes_per_block)
		fixed_probes_count = probes_per_block;
	sz_log(10, "Fetched %d real probes, fixed probes: %d", count, fixed_probes_count);
}

void szb_probeblock_combined_t::FetchProbes() {
	sz_log(10, "Fetching combined probes");
	TParam ** p_cache = param->GetFormulaCache();
	size_t msw_count = buffer->prober_connection->Get(start_time + SZBASE_PROBE_SPAN * fixed_probes_count,
			end_time,
			p_cache[0]->GetSzbaseName());
	if (!msw_count)
		return;
	short buf_msw[msw_count];
	buffer->prober_connection->GetData(buf_msw, msw_count);
	time_t server_time = buffer->prober_connection->GetServerTime();
	size_t lsw_count = buffer->prober_connection->Get(start_time + SZBASE_PROBE_SPAN * fixed_probes_count,
			end_time,
			p_cache[1]->GetSzbaseName());
	if (!lsw_count)
		return;
	short buf_lsw[lsw_count];
	buffer->prober_connection->GetData(buf_lsw, lsw_count);
	size_t count = std::min(lsw_count, msw_count);
	double prec10 = pow10(param->GetPrec());
	for (size_t i = 0; i < count; i++) {
		if (buf_msw[i] == SZB_FILE_NODATA)
			data[fixed_probes_count + i] = SZB_NODATA;
		else
			data[fixed_probes_count + i] = (SZBASE_TYPE) ( (double)(
				    int(szbfile_endian(buf_msw[i]) << 16)
				    	| (unsigned short)(szbfile_endian(buf_lsw[i]))
				    ) / prec10);
	}
	if (server_time < start_time)
		return;
	fixed_probes_count = (server_time - start_time) / SZBASE_PROBE_SPAN + 1;
	if (fixed_probes_count > probes_per_block)
		fixed_probes_count = probes_per_block;
	sz_log(10, "Fetched %d combined probes, fixed probes: %d", count, fixed_probes_count);
}

void szb_probeblock_definable_t::FetchProbes() {
	sz_log(10, "Fetching definable probes");
	time_t range_start, range_end;
	if (!buffer->prober_connection->GetRange(range_start, range_end))
		return;
	int count;
	if (range_end > GetEndTime())
		count = probes_per_block;
	else if (GetStartTime() > range_end) {
		sz_log(10, "Not fetching probes for definable block because it is from the future");
		return;
	} else
		count = (range_end - GetStartTime()) / SZBASE_PROBE_SPAN + 1;
	int num_of_params = param->GetNumParsInFormula();
	TParam** p_cache = param->GetFormulaCache();
	szb_lock_buffer(buffer);
	const double* dblocks[num_of_params];
	int new_fixed_probes = probes_per_block;
	if (num_of_params) for (int i = 0; i < num_of_params; i++) {
		szb_probeblock_t* block = szb_get_probeblock(buffer, p_cache[i], GetStartTime());
		dblocks[i] = block->GetData();
		new_fixed_probes = std::min(new_fixed_probes, block->GetFixedProbesCount());
	} else
		new_fixed_probes = count;
	double pw = pow(10, param->GetPrec());
	SZBASE_TYPE  stack[DEFINABLE_STACK_SIZE]; // stack for calculatinon of formula
	const std::wstring& formula = this->param->GetDrawFormula();
	for (int i = fixed_probes_count; i < count; i++) {
		time_t time = GetStartTime() + i * SZBASE_PROBE_SPAN;
		szbase_value_fetch_functor value_fetch(buffer, time, PT_SEC10);	
		default_is_summer_functor is_summer(time, param);
		data[i] = szb_definable_calculate(stack, DEFINABLE_STACK_SIZE, dblocks, p_cache, formula, num_of_params, value_fetch, is_summer, param) / pw;
		for (int j = 0; j < num_of_params; j++)
			if (dblocks[j])
				dblocks[j] += 1;
	}
	fixed_probes_count = new_fixed_probes;
	szb_unlock_buffer(buffer);
	sz_log(10, "Fetched %d definable probes, fixed probes: %d", count, fixed_probes_count);
}

void szb_probeblock_lua_t::FetchProbes() {
	sz_log(10, "Fetcheing lua probes");
	time_t range_start, range_end;
	if (!buffer->prober_connection->GetRange(range_start, range_end))
		return;

	time_t param_start_time = param->GetLuaStartDateTime() > 0 ? param->GetLuaStartDateTime() : range_start; 
	param_start_time += param->GetLuaStartOffset();
	if (param_start_time > GetEndTime())
		return;

	range_end += param->GetLuaEndOffset();
	if (range_end < GetStartTime())
		return;

	lua_State* lua = Lua::GetInterpreter();

	int ref = param->GetLuaParamReference();
	if (ref == LUA_REFNIL)
		return;
	if (ref == LUA_NOREF) {
		ref = lua::compile_lua_param(lua, param);
		if (ref == LUA_REFNIL) {
			buffer->last_err = SZBE_LUA_ERROR;
			buffer->last_err_string = SC::lua_error2szarp(lua_tostring(lua, -1));
			lua_pop(lua, 1);
			return;
		}

	}

	lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);
	time_t t = start_time + fixed_probes_count * SZBASE_PROBE_SPAN;
	double prec10 = pow10(param->GetPrec());
	for (int i = fixed_probes_count; i < probes_per_block && t < range_end; i++, t += SZBASE_PROBE_SPAN) {
		if (t < param_start_time) { 
			fixed_probes_count++;
			continue;
		}

		Lua::fixed.push(true);
		lua_get_val(lua, buffer, t, PT_SEC10, 0, data[i]);
		data[i] = rint(data[i] * prec10) / prec10; 
		bool fixedvalue = Lua::fixed.top();
		Lua::fixed.pop();
		if (fixedvalue && fixed_probes_count == i)
			fixed_probes_count += 1;
	}
	lua_pop(lua, 1);
	sz_log(10, "Fetched lua probes, fixed probes: %d", fixed_probes_count);
}

void szb_probeblock_lua_opt_t::FetchProbes() {
	sz_log(10, "Fetcheing lua probes");
	time_t range_start, range_end;
	if (!buffer->prober_connection->GetRange(range_start, range_end))
		return;

	time_t param_start_time = param->GetLuaStartDateTime() > 0 ? param->GetLuaStartDateTime() : range_start; 
	param_start_time += param->GetLuaStartOffset();
	if (param_start_time > GetEndTime())
		return;

	range_end += param->GetLuaEndOffset();
	if (range_end < GetStartTime())
		return;

	LuaExec::SzbaseExecutionEngine ee(buffer, param->GetLuaExecParam());
	sz_log(10, "Fetching lua opt probes");
	time_t t = start_time + fixed_probes_count * SZBASE_PROBE_SPAN;
	for (int i = fixed_probes_count; i < probes_per_block && t < range_end; i++, t += SZBASE_PROBE_SPAN) {
		if (t < param_start_time) { 
			fixed_probes_count++;
			continue;
		}

		bool probe_fixed = true;
		ee.CalculateValue(t, PT_SEC10, data[i], probe_fixed);
		if (probe_fixed && fixed_probes_count == i)
			fixed_probes_count = i + 1;
	}
	sz_log(10, "Fetched lua opt probes fixed probes: %d", fixed_probes_count);

}


#if LUA_PARAM_OPTIMISE
szb_probeblock_t* create_lua_probe_block(szb_buffer_t *b, TParam *p, time_t t) {
	LuaExec::Param *ep = p->GetLuaExecParam();
	if (ep == NULL) {
		ep = LuaExec::optimize_lua_param(p);
		b->AddExecParam(p);
	}
	if (ep->m_optimized)
		return new szb_probeblock_lua_opt_t(b, p, t);
	else
		return new szb_probeblock_lua_t(b, p, t);


}
#else
szb_probeblock_t *create_lua_data_block(szb_buffer_t *b, TParam* p, time_t t) {
	return new szb_probeblock_lua_t(b, p, t);
}
#endif

szb_probeblock_t* szb_create_probe_block(szb_buffer_t *buffer, TParam *param, time_t time) {
	szb_probeblock_t* ret = NULL;

	if (param->IsDefinable())
		param->PrepareDefinable();

	switch (param->GetType()) {
		case TParam::P_REAL:
			ret = new szb_probeblock_real_t(buffer, param, time);
			break;
		case TParam::P_COMBINED:
			ret = new szb_probeblock_combined_t(buffer, param, time);
			break;
		case TParam::P_DEFINABLE:
			ret = new szb_probeblock_definable_t(buffer, param, time);
			break;
#ifndef NO_LUA
		case TParam::P_LUA:
			if (param->GetFormulaType() == TParam::LUA_AV)
				return NULL;
			ret = create_lua_probe_block(buffer, param, time);
			break;
#endif
		default:
			fprintf(stderr,  "szb_calculate_block_default\n");
			ret = NULL;
			break;
	}
	return ret;
}

