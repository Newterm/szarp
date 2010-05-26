#include "config.h"

#include <boost/asio.hpp>

#include "szbbuf.h"
#include "szbdefines.h"
#include "szbdate.h"
#include "definablecalculate.h"
#include "luacalculate.h"
#include "loptcalculate.h"
#include "proberconnection.h"
#include "szbbase.h"
#include "conversion.h"


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
	szb_probeblock_real_t(szb_buffer_t *buffer, TParam* param, time_t start_time) : szb_probeblock_t(buffer, param, start_time, start_time * (probes_per_block - 1) * SZBASE_PROBE_SPAN) {}
};

class szb_probeblock_combined_t : public szb_probeblock_t {

protected:
	virtual void FetchProbes(); 
public:
	szb_probeblock_combined_t(szb_buffer_t *buffer, TParam* param, time_t start_time) : szb_probeblock_t(buffer, param, start_time, start_time * (probes_per_block - 1) * SZBASE_PROBE_SPAN) {}
};

class szb_probeblock_definable_t : public szb_probeblock_t {

protected:
	virtual void FetchProbes(); 
public:
	szb_probeblock_definable_t(szb_buffer_t *buffer, TParam* param, time_t start_time) : szb_probeblock_t(buffer, param, start_time, start_time * (probes_per_block - 1) * SZBASE_PROBE_SPAN) {}
};

class szb_probeblock_lua_t : public szb_probeblock_t {

protected:
	virtual void FetchProbes(); 
public:
	szb_probeblock_lua_t(szb_buffer_t *buffer, TParam* param, time_t start_time) : szb_probeblock_t(buffer, param, start_time, start_time * (probes_per_block - 1) * SZBASE_PROBE_SPAN) {}
};

class szb_probeblock_lua_opt_t : public szb_probeblock_t {

protected:
	virtual void FetchProbes(); 
public:
	szb_probeblock_lua_opt_t(szb_buffer_t *buffer, TParam* param, time_t start_time) : szb_probeblock_t(buffer, param, start_time, start_time * (probes_per_block - 1) * SZBASE_PROBE_SPAN) {}
};


void szb_probeblock_real_t::FetchProbes() {
	size_t count = buffer->prober_connection->Get(start_time + SZBASE_PROBE_SPAN * fixed_probes_count,
			end_time,
			param->GetSzbaseName());
	if (!count)
		return;

	short values[count];
	buffer->prober_connection->GetData(values, count);
 
	int prec10 = pow10(param->GetPrec());
	for (size_t i = 0; i < count; i++)
		data[i + fixed_probes_count] =  prec10 * values[i];
	
	time_t server_time = buffer->prober_connection->GetServerTime();
	if (server_time < start_time)
		return;

	fixed_probes_count = (buffer->prober_connection->GetServerTime() - start_time) / SZBASE_PROBE_SPAN + 1;
	if (fixed_probes_count > probes_per_block)
		fixed_probes_count = probes_per_block;
}

void szb_probeblock_combined_t::FetchProbes() {
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
	size_t count = std::min(lsw_count, msw_count);
	double prec10 = pow10(param->GetPrec());
	for (size_t i = 0; i < count; i++) {
		if (buf_msw[i] == SZB_FILE_NODATA)
			data[fixed_probes_count + i] = SZB_NODATA;
		else
			data[i] = (SZBASE_TYPE) ( (double)(
				    int(szbfile_endian(buf_msw[i]) << 16)
				    	| (unsigned short)(szbfile_endian(buf_lsw[i]))
				    ) / prec10);
	}
	if (server_time < start_time)
		return;
	fixed_probes_count = (buffer->prober_connection->GetServerTime() - start_time) / SZBASE_PROBE_SPAN + 1;
	if (fixed_probes_count > probes_per_block)
		fixed_probes_count = probes_per_block;
}

void szb_probeblock_definable_t::FetchProbes() {
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
	size_t count = std::min(lsw_count, msw_count);
	double prec10 = pow10(param->GetPrec());
	for (size_t i = 0; i < count; i++) {
		if (buf_msw[i] == SZB_FILE_NODATA)
			data[fixed_probes_count + i] = SZB_NODATA;
		else
			data[i] = (SZBASE_TYPE) ( (double)(
				    int(szbfile_endian(buf_msw[i]) << 16)
				    	| (unsigned short)(szbfile_endian(buf_lsw[i]))
				    ) / prec10);
	}
	if (server_time < start_time)
		return;
	fixed_probes_count = (buffer->prober_connection->GetServerTime() - start_time) / SZBASE_PROBE_SPAN + 1;
	if (fixed_probes_count > probes_per_block)
		fixed_probes_count = probes_per_block;
}

void szb_probeblock_lua_t::FetchProbes() {
	lua_State* lua = Lua::GetInterpreter();
	time_t t = start_time + fixed_probes_count * SZBASE_PROBE_SPAN;
	int ref = param->GetLuaParamReference();

	if (ref == LUA_REFNIL)
		return;

	if (ref == LUA_NOREF) {
		ref = compile_lua_param(lua, param);
		if (ref == LUA_REFNIL) {
			buffer->last_err = SZBE_LUA_ERROR;
			buffer->last_err_string = SC::U2S((const unsigned char*)lua_tostring(lua, -1));
			lua_pop(lua, 1);
			return;
		}

	}

	lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);
	double prec10 = pow10(param->GetPrec());
	for (int i = fixed_probes_count; i < probes_per_block; i++, t += SZBASE_PROBE_SPAN) {
		Lua::fixed.push(true);
		lua_get_val(lua, buffer, t, PT_SEC10, 0, data[i]);
		data[i] = rint(data[i] * prec10) / prec10; 
		bool fixedvalue = Lua::fixed.top();
		Lua::fixed.pop();
		if (fixedvalue && fixed_probes_count == i)
			fixed_probes_count += 1;
	}
	lua_pop(lua, 1);
}

void szb_probeblock_lua_opt_t::FetchProbes() {
	LuaExec::ExecutionEngine ee(buffer, param->GetLuaExecParam());

	time_t t = start_time + fixed_probes_count * SZBASE_PROBE_SPAN;
	for (int i = fixed_probes_count; i < probes_per_block; i++, t += SZBASE_PROBE_SPAN) {
		bool probe_fixed = true;
		ee.CalculateValue(t, PT_SEC10, data[i], probe_fixed);
		if (probe_fixed && fixed_probes_count == i)
			fixed_probes_count = i + 1;
	}

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

time_t 
szb_real_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle) {
	if (buffer->PrepareConnection() == false)
		return -1;
	time_t t = buffer->prober_connection->Search(start, end, direction, param->GetSzbaseName());
	if (t == (time_t) -1)
		if (buffer->prober_connection->IsError())
			buffer->prober_connection->ClearError();
	return t;
}

time_t 
szb_combined_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle) {
	if (buffer->PrepareConnection() == false)
		return -1;
	TParam ** p_cache = param->GetFormulaCache();
	time_t msw_ret = szb_real_search_probe(buffer, p_cache[0], start, end, direction, c_handle);
	if (msw_ret == (time_t) -1)
		return msw_ret;

	time_t lsw_ret = szb_real_search_probe(buffer, p_cache[1], start, end, direction, c_handle);
	if (lsw_ret == (time_t) -1)
		return lsw_ret;

	if (direction < 0)
		return std::min(lsw_ret, msw_ret);
	else
		return std::max(lsw_ret, msw_ret);
}


time_t search_in_range(szb_buffer_t* buffer, TParam* param, time_t start, time_t end, int direction) {
	if (param->IsConst())
		return start;

	if (direction == 0)
		return IS_SZB_NODATA(szb_get_probe(buffer, param, start, PT_SEC10)) ? -1 : start;

	szb_block_t *block = NULL;
	for (time_t t = start; direction > 0 ? t <= start: t >= end; t += SZBASE_PROBE_SPAN * direction) {
		if (block == NULL || block->GetStartTime() > t || block->GetEndTime() < t) {
			time_t b_start = t
				/ (SZBASE_PROBE_SPAN * szb_probeblock_t::probes_per_block)
				* (SZBASE_PROBE_SPAN * szb_probeblock_t::probes_per_block);
			block = szb_get_block(buffer, 
						param,
						b_start,
						SEC10_BLOCK);
			if (buffer->last_err)
				return -1;
		}

		int index = (t - block->GetStartTime()) / SZBASE_PROBE_SPAN;
		if (!IS_SZB_NODATA(block->GetData()[index]))
			return t;
	}

	return -1;
}

bool adjust_search_boundaries(time_t& start, time_t& end, time_t first_date, time_t last_date, int direction) {
	if (direction <= 0 && start < first_date)
		return false;

	if (direction >= 0 && start > last_date)
		return false;

	if (direction > 0 && start < first_date)
		start = first_date;

	if (direction < 0 && start > last_date)
		start = first_date;

	if (end == -1) {
		if (direction < 0)
			end = first_date;
		else
			end = last_date;
	}

	return true;
}

time_t 
szb_definable_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle) {
	time_t first_date;
	time_t last_date;

	if (buffer->prober_connection->GetBoundaryTimes(first_date, last_date) == false)
		return -1;

	if (adjust_search_boundaries(start, end, first_date, last_date, direction) == false)
		return -1;

	return search_in_range(buffer, param, start, end, direction);
}

time_t 
szb_lua_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle) {
	time_t first_date;
	time_t last_date;

	if (buffer->prober_connection->GetBoundaryTimes(first_date, last_date) == false)
		return -1;
	
	if (param->GetLuaStartDateTime() > 0)
		first_date = param->GetLuaStartDateTime();

	first_date += param->GetLuaStartOffset();
	last_date += param->GetLuaEndOffset();

	if (adjust_search_boundaries(start, end, first_date, last_date, direction) == false)
		return -1;

	return search_in_range(buffer, param, start, end, direction);
}
