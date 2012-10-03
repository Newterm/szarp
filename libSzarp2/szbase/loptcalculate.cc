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

#ifdef LUA_PARAM_OPTIMISE

#include "conversion.h"
#include <boost/make_shared.hpp>

#include <cmath>
#include <cfloat>
#include <functional>
#include <boost/variant.hpp>

#include "conversion.h"
#include "liblog.h"
#include "szarp_base_common/lua_syntax.h"
#include "szbbase.h"
#include "loptcalculate.h"

namespace LuaExec {

using namespace lua_grammar;

class SzbaseExecutionEngine;

SzbaseExecutionEngine::SzbaseExecutionEngine(szb_buffer_t *buffer, Param *param) {
	m_buffer = buffer;
	szb_lock_buffer(m_buffer);
	m_param = param;
	Szbase* szbase = Szbase::GetObject();
	for (size_t i = 0; i < m_param->m_par_refs.size(); i++) {
		m_param->m_par_refs[i].PushExecutionEngine(this);
		m_buffers.push_back(szbase->GetBufferForParam(m_param->m_par_refs[i].m_param));
	}
	m_blocks.resize(UNUSED_BLOCK_TYPE);
	m_blocks_iterators.resize(UNUSED_BLOCK_TYPE);
	for (size_t i = 0 ; i < UNUSED_BLOCK_TYPE; i++) {
		m_blocks[i].resize(m_param->m_par_refs.size());
		m_blocks_iterators[i].resize(m_param->m_par_refs.size());
	}
	m_vals.resize(m_param->m_vars.size());
	for (size_t i = 0; i < m_vals.size(); i++)
		m_param->m_vars[i].PushExecutionEngine(this);
	m_vals[3] = PT_MIN10;
	m_vals[4] = PT_HOUR;
	m_vals[5] = PT_HOUR8;
	m_vals[6] = PT_DAY;
	m_vals[7] = PT_WEEK;
	m_vals[8] = PT_MONTH;
	m_vals[9] = PT_SEC10;
}

void SzbaseExecutionEngine::CalculateValue(time_t t, SZARP_PROBE_TYPE probe_type, double &val, bool &fixed) {
	m_fixed = true;
	m_vals[0] = nan("");
	m_vals[1] = t;
	m_vals[2] = probe_type;
	m_param->m_statement->Execute();
	fixed = m_fixed;
	val = m_vals[0];
}

SzbaseExecutionEngine::ListEntry SzbaseExecutionEngine::GetBlockEntry(size_t param_index, time_t t, SZB_BLOCK_TYPE bt) {
	ListEntry le;
	le.block = szb_get_block(m_buffers[param_index], m_param->m_par_refs[param_index].m_param, t, bt);
	int year, month;
	switch (bt) {
		case MIN10_BLOCK:
			szb_time2my(t, &year, &month);
			le.start_time = probe2time(0, year, month);
			le.end_time = le.start_time + SZBASE_DATA_SPAN * szb_probecnt(year, month);	
			break;
		case SEC10_BLOCK:
			le.start_time = szb_round_to_probe_block_start(t);
			le.end_time = le.start_time + SZBASE_PROBE_SPAN * szb_probeblock_t::probes_per_block;
			break;
		case UNUSED_BLOCK_TYPE:
			assert(false);
	}
	//force retrieval/refreshment of data in block
	if (le.block)
		le.block->GetData();
	return le;
}

szb_block_t* SzbaseExecutionEngine::AddBlock(size_t param_index,
		time_t t,
		std::list<ListEntry>::iterator& i,
		SZB_BLOCK_TYPE bt) {
	ListEntry le(GetBlockEntry(param_index, t, bt));
	m_blocks[bt][param_index].insert(i, le);
	std::advance(i, -1);
	return i->block;
}

szb_block_t* SzbaseExecutionEngine::SearchBlockLeft(size_t param_index,
		time_t t,
		std::list<ListEntry>::iterator& i,
		SZB_BLOCK_TYPE bt) {
	do {
		if (i == m_blocks[bt][param_index].begin())
			return AddBlock(param_index, t, i, bt);

		if ((--i)->end_time <= t)
			return AddBlock(param_index, t, ++i, bt);

		if (i->start_time <= t)
			return i->block;
		
	} while (true);	

}

szb_block_t* SzbaseExecutionEngine::SearchBlockRight(
		size_t param_index,
		time_t t, std::list<ListEntry>::iterator& i,
		SZB_BLOCK_TYPE bt) {
	do {
		if (++i == m_blocks[bt][param_index].end())
			return AddBlock(param_index, t, i, bt);

		if (i->start_time > t)
			return AddBlock(param_index, t, i, bt);

		if (i->end_time > t)
			return i->block;
		
	} while (true);	

}

szb_block_t* SzbaseExecutionEngine::GetBlock(size_t param_index,
		time_t time,
		SZB_BLOCK_TYPE bt) {
	if (m_blocks[bt][param_index].size()) {	
		std::list<ListEntry>::iterator& i = m_blocks_iterators[bt][param_index];
		if (i->start_time <= time && time < i->end_time)
			return i->block;
		else if (i->start_time > time)
			return SearchBlockLeft(param_index, time, i, bt);
		else
			return SearchBlockRight(param_index, time, i, bt);
	} else {
		ListEntry le(GetBlockEntry(param_index, time, bt));
		m_blocks[bt][param_index].push_back(le);
		m_blocks_iterators[bt][param_index] = m_blocks[bt][param_index].begin();
		return m_blocks_iterators[bt][param_index]->block;
	}
}

double SzbaseExecutionEngine::ValueBlock(ParamRef& ref, const time_t& time, SZB_BLOCK_TYPE block_type) {
	double ret;

	szb_block_t* block = GetBlock(ref.m_param_index, time, block_type);
	if (block) {
		time_t timediff = time - block->GetStartTime();
		int probe_index;
		switch (block_type) {
			case MIN10_BLOCK:
				probe_index = timediff / SZBASE_DATA_SPAN;
				break;
			case SEC10_BLOCK:
				probe_index = timediff / SZBASE_PROBE_SPAN;
				break;
			default:
				assert(false);
		}
		if (block->GetFixedProbesCount() <= probe_index)
			m_fixed = false;
		ret = block->GetData(false)[probe_index];
#ifdef LUA_OPTIMIZER_DEBUG
		if (std::isnan(ret) && m_fixed) {
			lua_opt_debug_stream << "Lua opt - fixed no data value, probe_index: " << probe_index << std::endl;
			lua_opt_debug_stream << "from param: " << SC::S2A(block.block->param->GetName()) << std::endl;
			lua_opt_debug_stream << "root dir: " << SC::S2A(block.block->buffer->rootdir) << std::endl;
		}
#endif
	} else {
		m_fixed = false;
		ret = nan("");
	}
	return ret;
}

double SzbaseExecutionEngine::ValueAvg(ParamRef& ref, const time_t& time, const double& period_type) {
	bool fixed;
	time_t ptime = szb_round_time(time, (SZARP_PROBE_TYPE) period_type, 0);
	double ret = szb_get_avg(m_buffers[ref.m_param_index], ref.m_param, ptime, szb_move_time(ptime, 1, (SZARP_PROBE_TYPE)period_type, 0), NULL, NULL, (SZARP_PROBE_TYPE)period_type, &fixed);
	if (!fixed)
		m_fixed = false;
	return ret;
}

double SzbaseExecutionEngine::Value(size_t param_index, const double& time_, const double& period_type) {
	time_t time = time_;

	ParamRef& ref = m_param->m_par_refs[param_index];

	if (ref.m_param->GetFormulaType() == TParam::LUA_AV)
		return ValueAvg(ref, time, period_type);

	switch ((SZARP_PROBE_TYPE) period_type) {
		case PT_MIN10: 
			return ValueBlock(ref, time, MIN10_BLOCK);
		case PT_SEC10:
			return ValueBlock(ref, time, SEC10_BLOCK);
		default: 
			return ValueAvg(ref, time, period_type);
	}
}

std::vector<double>& SzbaseExecutionEngine::Vars() {
	return m_vals;
}

SzbaseExecutionEngine::~SzbaseExecutionEngine() {
	szb_unlock_buffer(m_buffer);
	for (size_t i = 0; i < m_param->m_par_refs.size(); i++)
		m_param->m_par_refs[i].PopExecutionEngine();
	for (size_t i = 0; i < m_vals.size(); i++)
		m_param->m_vars[i].PopExecutionEngine();
}

Param* optimize_lua_param(TParam* p) {
	LuaExec::SzbaseParam* ep = new LuaExec::SzbaseParam;
	p->SetLuaExecParam(ep);

	std::wstring param_text = SC::U2S(p->GetLuaScript());
	std::wstring::const_iterator param_text_begin = param_text.begin();
	std::wstring::const_iterator param_text_end = param_text.end();

	ep->m_optimized = false;

	lua_grammar::chunk param_code;
	if (lua_grammar::parse(param_text_begin, param_text_end, param_code)) {
		LuaExec::ParamConverter pc(IPKContainer::GetObject());
		try {
			pc.ConvertParam(param_code, ep);
			ep->m_optimized = true;
			for (std::vector<LuaExec::ParamRef>::iterator i = ep->m_par_refs.begin();
				 	i != ep->m_par_refs.end();
					i++)
				Szbase::GetObject()->AddLuaOptParamReference(i->m_param, p);
			//no params are referenced by this param
			if (ep->m_par_refs.size() == 0) {
				szb_buffer_t* buffer = Szbase::GetObject()->GetBuffer(p->GetSzarpConfig()->GetPrefix());
				assert(buffer);
				ep->m_last_update_times[buffer] = -1;
			}
		} catch (LuaExec::ParamConversionException &e) {
			sz_log(3, "Parameter %ls cannot be optimized, reason: %ls", p->GetName().c_str(), e.what().c_str());
#ifdef LUA_OPTIMIZER_DEBUG
			lua_opt_debug_stream << "Parameter " << SC::S2A(p->GetName()) << " cannot be optimized, reason: " << SC::S2A(e.what()) << std::endl;
#endif
		}
	}
	return ep;
}

} // LuaExec
#endif
