#ifndef __SZ4_LUA_OPTIMIZED_PARAM_ENTRY_H__
#define __SZ4_LUA_OPTIMIZED_PARAM_ENTRY_H__
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

#include "szarp_base_common/lua_param_optimizer.h"
#include "sz4/definable_param_cache.h"
#include "sz4/buffered_param_entry.h"
#include "sz4/fixed_stack_top.h"

namespace sz4 {

template<class types> class execution_engine : public LuaExec::ExecutionEngine {
	base_templ<types>* m_base;
	LuaExec::Param* m_exec_param;
	std::vector<double> m_vars;
	bool m_initialized;

	void initialize() {
		m_vars.resize(m_exec_param->m_vars.size());

		m_vars[3] = PT_MIN10;
		m_vars[4] = PT_HOUR;
		m_vars[5] = PT_HOUR8;
		m_vars[6] = PT_DAY;
		m_vars[7] = PT_WEEK;
		m_vars[8] = PT_MONTH;
		m_vars[9] = PT_SEC10;
	
		for (size_t i = 0; i < m_vars.size(); i++)
			m_exec_param->m_vars[i].PushExecutionEngine(this);
		for (size_t i = 0; i < m_exec_param->m_par_refs.size(); i++)
			m_exec_param->m_par_refs[i].PushExecutionEngine(this);

	}
public:
	execution_engine(base_templ<types>* _base, TParam* param) : m_base(_base), m_exec_param(param->GetLuaExecParam()), m_initialized(false) {
	}

	std::pair<double, bool> calculate_value(second_time_t time, SZARP_PROBE_TYPE probe_type) {
		if (!m_initialized) {
			initialize();
			m_initialized = true;
		}

		fixed_stack_top stack_top(m_base->fixed_stack());

		m_vars[0] = nan("");
		m_vars[1] = time;
		m_vars[2] = probe_type;
		m_exec_param->m_statement->Execute();

		return std::make_pair(m_vars[0], stack_top.value());
	}

	virtual double Value(size_t param_index, const double& time_, const double& probe_type) {
		LuaExec::ParamRef& ref = m_exec_param->m_par_refs[param_index];
		//this probably should depend on type of parameter we are calucating data for
		weighted_sum<double, nanosecond_time_t> sum;
		m_base->get_weighted_sum(ref.m_param,
				time_,
				szb_move_time(time_, 1, SZARP_PROBE_TYPE(probe_type), 0),
				SZARP_PROBE_TYPE(probe_type),
				sum);

		assert(m_base->fixed_stack().size());
		m_base->fixed_stack().top() &= sum.fixed();

		if (sum.weight())
			return sum.sum() / sum.weight();
		else
			return nan("");
	}

	virtual std::vector<double>& Vars() {
		return m_vars;
	}

	~execution_engine() {
		if (!m_initialized)
			return;
		for (size_t i = 0; i < m_vars.size(); i++)
			m_exec_param->m_vars[i].PopExecutionEngine();
		for (size_t i = 0; i < m_exec_param->m_par_refs.size(); i++)
			m_exec_param->m_par_refs[i].PopExecutionEngine();
	}
};

template<class value_type, class time_type, class types> class lua_optimized_param_entry_in_buffer : public buffered_param_entry_in_buffer<value_type, time_type, types, execution_engine> {
public:
	lua_optimized_param_entry_in_buffer(base_templ<types>* _base, TParam* param, const boost::filesystem::wpath& path) : buffered_param_entry_in_buffer<value_type, time_type, types, execution_engine>(_base, param, path) {}
};

}
#endif

