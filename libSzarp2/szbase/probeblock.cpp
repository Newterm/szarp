#include "probeblock.h"


class szb_probes_block_real {

protected:
	virtual void Fetch() = 0; 
public:
	szb_probes_block_real(szb_buffer_t *m_buffer, TParam* param);
};

class szb_probes_block_combined {

protected:
	virtual void Fetch() = 0; 
public:
	szb_probes_block_combined(szb_buffer_t *m_buffer, TParam* param);
};


szb_probes_block_real::Fetch() {
	size_t count = m_buffer->m_connection->Get(m_start_time + PROBE_TIME * m_non_fixed_probe,
			end,
			param->GetSzbasePath());
	if (!count)
		return;

	short values[count];
	m_buffer->m_connection->GetData(values, count);
 
	int prec10 = pow10(param->GetPrec());
	for (size_t i = 0; i < fetched i++)
		m_data[i + m_non_fixed_probe] =  prec10 * values[i];
	
	time_t server_time = m_buffer->m_connection->GetServerTime();
	if (server_time < m_start_time)
		return;

	m_non_fixed_probe = (m_buffer->m_connection->GetServerTime() - m_start_time) / PROBE_TIME + 1;
	if (m_non_fixed_probe > max_probes)
		m_non_fixed_probe = max_probes;
}

szb_probes_block_combined::Fetch() {
	TParam ** p_cache = param->GetFormulaCache();
	size_t msw_count = m_buffer->m_connection->Get(m_start_time + PROBE_TIME * m_non_fixed_probe,
			end,
			p_cache[0]->GetSzbasePath());
	if (!msw_count)
		return;
	short buf_msw[msw_count];
	m_buffer->m_connection->GetData(buf_msw, msw_count);
	time_t server_time = m_buffer->m_connection->GetServerTime();
	size_t lsw_count = m_buffer->m_connection->Get(m_start_time + PROBE_TIME * m_non_fixed_probe,
			end,
			p_cache[1]->GetSzbasePath());
	if (!lsw_count)
		return;
	short buf_lsw[lsw_count];
	size_t count = std::min(lsw_count, msw_count);
	double prec10 = pow10(m_param->GetPrec());
	for (size_t i = 0; i < count; i++) {
		if (buf_msw[i] == SZB_FILE_NODATA)
			m_data[m_non_fixed_probe + i] = SZB_NODATA;
		else
			m_data[i] = (SZBASE_TYPE) ( (double)(
				    int(szbfile_endian(buf_msw[i]) << 16)
				    	| (unsigned short)(szbfile_endian(buf_lsw[i]))
				    ) / prec10);
	}
	if (server_time < m_start_time)
		return;
	m_non_fixed_probe = (m_buffer->m_connection->GetServerTime() - m_start_time) / PROBE_TIME + 1;
	if (m_non_fixed_probe > max_probes)
		m_non_fixed_probe = max_probes;
}

szb_probes_block_combined::Fetch() {
	TParam ** p_cache = param->GetFormulaCache();
	size_t msw_count = m_buffer->m_connection->Get(m_start_time + PROBE_TIME * m_non_fixed_probe,
			end,
			p_cache[0]->GetSzbasePath());
	if (!msw_count)
		return;
	short buf_msw[msw_count];
	m_buffer->m_connection->GetData(buf_msw, msw_count);
	time_t server_time = m_buffer->m_connection->GetServerTime();
	size_t lsw_count = m_buffer->m_connection->Get(m_start_time + PROBE_TIME * m_non_fixed_probe,
			end,
			p_cache[1]->GetSzbasePath());
	if (!lsw_count)
		return;
	short buf_lsw[lsw_count];
	size_t count = std::min(lsw_count, msw_count);
	double prec10 = pow10(m_param->GetPrec());
	for (size_t i = 0; i < count; i++) {
		if (buf_msw[i] == SZB_FILE_NODATA)
			m_data[m_non_fixed_probe + i] = SZB_NODATA;
		else
			m_data[i] = (SZBASE_TYPE) ( (double)(
				    int(szbfile_endian(buf_msw[i]) << 16)
				    	| (unsigned short)(szbfile_endian(buf_lsw[i]))
				    ) / prec10);
	}
	if (server_time < m_start_time)
		return;
	m_non_fixed_probe = (m_buffer->m_connection->GetServerTime() - m_start_time) / PROBE_TIME + 1;
	if (m_non_fixed_probe > max_probes)
		m_non_fixed_probe = max_probes;
}

szb_probes_block_defined::Fetch() {
	int num_of_params = m_param->GetNumParsInFormula();
	TParam ** f_cache = m_param->GetFormulaCache();
	size_t fixed_count = m_non_fixed_probe;
	double* blocks[num_of_params];
	SZBASE_TYPE stack[DEFINABLE_STACK_SIZE]; // stack for calculatinon of formula

	for (size_t i = 0; i < num_of_params; i++) {
		szb_probes_block* block = szb_get_probes_block(m_buffer, f_cache[i], m_start_time);
		if (block == NULL)
			return;
		blocks[i] = block->m_data;
		blocks[i] = block;
		fixed_count = std::min(fixed_count, m_non_fixed_probe);
	}

	const std::wstring& formula = param->GetDrawFormula();
	time_t t = m_start_time + first_non_fixed_probe * PROBE_TIME;
	for (size_t i = first_non_fixed_probe; i < fixed_count; i++, t += PROBE_TIME)
		m_data[i] = szb_definable_calculate(buffer,
				stack,
				blocks,
				f_cache,
				formula,
				i,
				num_of_params);

}

void szb_probes_block_lua::Fetch() {
	lua_State* lua = Lua::GetInterpreter();
	time_t t = m_start_time + first_non_fixed_probe * PROBE_TIME;
	int ref = m_param->GetLuaParamReference();

	if (ref == LUA_REFNIL)
		return;

	if (ref == LUA_NOREF) {
		ref = compile_lua_param(lua, m_param);
		if (ref == LUA_REFNIL) {
			buffer->last_err = SZBE_LUA_ERROR;
			buffer->last_err_string = SC::U2S((const unsigned char*)lua_tostring(lua, -1));
			lua_pop(lua, 1);
			return;
		}

	}

	lua_rawgeti(lua, LUA_REGISTRYINDEX, ref);
	double prec10 = pow10(m_param->GetPrec());
	for (size_t i = m_non_fixed_probe; i < probes_per_block; i++, t += PROBE_TIME) {
		Lua::fixed.push(true);
		lua_get_val(lua, buffer, t, PT_SEC10, 0, m_data[i]);
		m_data[i] = rint(m_data[i] * prec10) / prec10; 
		bool fixedvalue = Lua::fixed.top();
		Lua::fixed.pop();
		if (fixedvalue && m_non_fixed_probe == i)
			m_non_fixed_probe += 1
	}
	lua_pop(lua, 1);
}

void szb_probes_block_lua_opt::Fetch() {


}
