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
#include "sz4/util.h"
#include "sz4/lua_first_last_time.h"
#include "sz4/lua_average_calc_algo.h"

namespace sz4 {

template<class types> class execution_engine : public LuaExec::ExecutionEngine, public lua_average_calc_algo {
	base_templ<types>* m_base;
	LuaExec::Param* m_exec_param;
	std::vector<double> m_vars;
	bool m_initialized;

	void initialize() {
		if (m_initialized)
			return;

		m_vars.resize(m_exec_param->m_vars.size());

		m_vars[3] = PT_MIN10;
		m_vars[4] = PT_HOUR;
		m_vars[5] = PT_HOUR8;
		m_vars[6] = PT_DAY;
		m_vars[7] = PT_WEEK;
		m_vars[8] = PT_MONTH;
		m_vars[9] = PT_SEC10;
		m_vars[10] = PT_SEC;
		m_vars[11] = PT_HALFSEC;
	
		for (size_t i = 0; i < m_vars.size(); i++)
			m_exec_param->m_vars[i].PushExecutionEngine(this);
		for (size_t i = 0; i < m_exec_param->m_par_refs.size(); i++)
			m_exec_param->m_par_refs[i].PushExecutionEngine(this);

		m_initialized = true;
	}
public:

	typedef double(execution_engine::*compute_method)(LuaExec::ParamRef& ref, const double& time_, SZARP_PROBE_TYPE probe_type);

	execution_engine(base_templ<types>* _base, TParam* param) : m_base(_base), m_exec_param(param->GetLuaExecParam()), m_initialized(false) {
	}

	void do_calculate_value(second_time_t time, SZARP_PROBE_TYPE probe_type, double &result, bool& fixed) {

		fixed_stack_top stack_top(m_base->fixed_stack());

		m_vars[0] = nan("");
		m_vars[1] = time;
		m_vars[2] = probe_type;
		m_exec_param->m_statement->Execute();

		fixed &= stack_top.top();
		result = m_vars[0];
	}

	template<class DT, class TT> double compute(LuaExec::ParamRef& ref, const double& time_, SZARP_PROBE_TYPE probe_type) {
		weighted_sum<DT, TT> sum;

		TT t = TT(time_);
		TT end_time = szb_move_time(t, 1, probe_type, 0);
		if (end_time == t)
			end_time += 1;

		m_base->get_weighted_sum(ref.m_param, t, end_time, probe_type, sum);

		m_base->fixed_stack().back() = m_base->fixed_stack().back() & sum.fixed();

		if (sum.weight()) 
			return scale_value(double(sum.avg()), ref.m_param);
		else 
			return nan("");
	}

	static const compute_method compute_methods[TParam::LAST_TIME_TYPE + 1][TParam::LAST_DATA_TYPE + 1];

	virtual double Value(size_t param_index, const double& time_, const double& probe_type_) {
		LuaExec::ParamRef& ref = m_exec_param->m_par_refs[param_index];
		SZARP_PROBE_TYPE probe_type = SZARP_PROBE_TYPE(probe_type_);
		compute_method method = compute_methods[ref.m_param->GetTimeType()][ref.m_param->GetDataType()];
		return (this->*method)(ref, time_, probe_type);
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

template<class types>
const typename execution_engine<types>::compute_method
execution_engine<types>::compute_methods[TParam::LAST_TIME_TYPE + 1][TParam::LAST_DATA_TYPE + 1] = {
	{ &execution_engine<types>::compute<short, second_time_t>,
		&execution_engine<types>::compute<int, second_time_t>,
		&execution_engine<types>::compute<float, second_time_t>,
		&execution_engine<types>::compute<double, second_time_t> },
	{ &execution_engine<types>::compute<short, nanosecond_time_t>,
		&execution_engine<types>::compute<int, nanosecond_time_t>,
		&execution_engine<types>::compute<float, nanosecond_time_t>,
		&execution_engine<types>::compute<double, nanosecond_time_t> }
};

template<class value_type, class time_type, class types> class lua_optimized_param_entry_in_buffer : public buffered_param_entry_in_buffer<value_type, time_type, types, execution_engine> {
public:
	lua_optimized_param_entry_in_buffer(base_templ<types>* _base, TParam* param, const boost::filesystem::wpath& path) : buffered_param_entry_in_buffer<value_type, time_type, types, execution_engine>(_base, param, path) {}

	void get_first_time(std::list<generic_param_entry*>& referred_params, time_type &t) {
		this->m_base->get_heartbeat_first_time(this->m_param, t);
		lua_adjust_first_time(this->m_param, t);
	}

	void get_last_time(const std::list<generic_param_entry*>& referred_params, time_type &t) {
		this->m_base->get_heartbeat_last_time(this->m_param, t);
		lua_adjust_last_time(this->m_param, t);
	}
};

}
#endif

