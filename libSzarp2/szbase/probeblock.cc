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

SZB_BLOCK_TYPE szb_probeblock_t::GetBlockType() {
	return SEC10_BLOCK;
}

const SZBASE_TYPE* szb_probeblock_t::GetData(bool refresh) {
	if (refresh)
		Refresh();
	return data;
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
	size_t count = buffer->prober_connection->Get(start_time + SZBASE_PROBE_SPAN * non_fixed_probe,
			end_time,
			param->GetSzbaseName());
	if (!count)
		return;

	short values[count];
	buffer->prober_connection->GetData(values, count);
 
	int prec10 = pow10(param->GetPrec());
	for (size_t i = 0; i < count; i++)
		data[i + non_fixed_probe] =  prec10 * values[i];
	
	time_t server_time = buffer->prober_connection->GetServerTime();
	if (server_time < start_time)
		return;

	non_fixed_probe = (buffer->prober_connection->GetServerTime() - start_time) / SZBASE_PROBE_SPAN + 1;
	if (non_fixed_probe > probes_per_block)
		non_fixed_probe = probes_per_block;
}

void szb_probeblock_combined_t::FetchProbes() {
	TParam ** p_cache = param->GetFormulaCache();
	size_t msw_count = buffer->prober_connection->Get(start_time + SZBASE_PROBE_SPAN * non_fixed_probe,
			end_time,
			p_cache[0]->GetSzbaseName());
	if (!msw_count)
		return;
	short buf_msw[msw_count];
	buffer->prober_connection->GetData(buf_msw, msw_count);
	time_t server_time = buffer->prober_connection->GetServerTime();
	size_t lsw_count = buffer->prober_connection->Get(start_time + SZBASE_PROBE_SPAN * non_fixed_probe,
			end_time,
			p_cache[1]->GetSzbaseName());
	if (!lsw_count)
		return;
	short buf_lsw[lsw_count];
	size_t count = std::min(lsw_count, msw_count);
	double prec10 = pow10(param->GetPrec());
	for (size_t i = 0; i < count; i++) {
		if (buf_msw[i] == SZB_FILE_NODATA)
			data[non_fixed_probe + i] = SZB_NODATA;
		else
			data[i] = (SZBASE_TYPE) ( (double)(
				    int(szbfile_endian(buf_msw[i]) << 16)
				    	| (unsigned short)(szbfile_endian(buf_lsw[i]))
				    ) / prec10);
	}
	if (server_time < start_time)
		return;
	non_fixed_probe = (buffer->prober_connection->GetServerTime() - start_time) / SZBASE_PROBE_SPAN + 1;
	if (non_fixed_probe > probes_per_block)
		non_fixed_probe = probes_per_block;
}

void szb_probeblock_definable_t::FetchProbes() {
	TParam ** p_cache = param->GetFormulaCache();
	size_t msw_count = buffer->prober_connection->Get(start_time + SZBASE_PROBE_SPAN * non_fixed_probe,
			end_time,
			p_cache[0]->GetSzbaseName());
	if (!msw_count)
		return;
	short buf_msw[msw_count];
	buffer->prober_connection->GetData(buf_msw, msw_count);
	time_t server_time = buffer->prober_connection->GetServerTime();
	size_t lsw_count = buffer->prober_connection->Get(start_time + SZBASE_PROBE_SPAN * non_fixed_probe,
			end_time,
			p_cache[1]->GetSzbaseName());
	if (!lsw_count)
		return;
	short buf_lsw[lsw_count];
	size_t count = std::min(lsw_count, msw_count);
	double prec10 = pow10(param->GetPrec());
	for (size_t i = 0; i < count; i++) {
		if (buf_msw[i] == SZB_FILE_NODATA)
			data[non_fixed_probe + i] = SZB_NODATA;
		else
			data[i] = (SZBASE_TYPE) ( (double)(
				    int(szbfile_endian(buf_msw[i]) << 16)
				    	| (unsigned short)(szbfile_endian(buf_lsw[i]))
				    ) / prec10);
	}
	if (server_time < start_time)
		return;
	non_fixed_probe = (buffer->prober_connection->GetServerTime() - start_time) / SZBASE_PROBE_SPAN + 1;
	if (non_fixed_probe > probes_per_block)
		non_fixed_probe = probes_per_block;
}

void szb_probeblock_lua_t::FetchProbes() {
	lua_State* lua = Lua::GetInterpreter();
	time_t t = start_time + non_fixed_probe * SZBASE_PROBE_SPAN;
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
	for (size_t i = non_fixed_probe; i < probes_per_block; i++, t += SZBASE_PROBE_SPAN) {
		Lua::fixed.push(true);
		lua_get_val(lua, buffer, t, PT_SEC10, 0, data[i]);
		data[i] = rint(data[i] * prec10) / prec10; 
		bool fixedvalue = Lua::fixed.top();
		Lua::fixed.pop();
		if (fixedvalue && non_fixed_probe == i)
			non_fixed_probe += 1;
	}
	lua_pop(lua, 1);
}

void szb_probeblock_lua_opt_t::FetchProbes() {
	LuaExec::ExecutionEngine ee(buffer, param->GetLuaExecParam());

	time_t t = start_time + non_fixed_probe * SZBASE_PROBE_SPAN;
	for (size_t i = non_fixed_probe; i < probes_per_block; i++, t += SZBASE_PROBE_SPAN) {
		bool probe_fixed = true;
		ee.CalculateValue(t, PT_SEC10, data[i], probe_fixed);
		if (probe_fixed && non_fixed_probe == i)
			non_fixed_probe = i + 1;
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


