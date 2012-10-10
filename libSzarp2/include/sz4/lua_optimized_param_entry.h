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
#ifndef __SZ4_LUA_OPTIMIZED_PARAM_ENTRY_H__
#define __SZ4_LUA_OPTIMIZED_PARAM_ENTRY_H__

#include "szarp_base_common/lua_param_optimizer.h"
#include "sz4/definable_param_cache.h"

namespace sz4 {

class execution_engine : public LuaExec::ExecutionEngine {
	base* m_base;
	LuaExec::Param* m_exec_param;
	std::vector<double> m_vars;
public:
	execution_engine(base* _base, TParam* param) : m_base(_base) {
		m_exec_param = param->GetLuaExecParam();
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
	}

	double calculate_value(second_time_t time, SZARP_PROBE_TYPE probe_type) {
		m_vars[0] = nan("");
		m_vars[1] = time;
		m_vars[2] = probe_type;
		m_exec_param->m_statement->Execute();
		return m_vars[0];
	}

	virtual double Value(size_t param_index, const double& time_, const double& probe_type) {
		LuaExec::ParamRef& ref = m_exec_param->m_par_refs[param_index];
		//XXX
		weighted_sum<double, second_time_t> sum;
		m_base->get_weighted_sum(ref.m_param, second_time_t(time_), second_time_t(szb_move_time(time_, 1, SZARP_PROBE_TYPE(probe_type), 0)), SZARP_PROBE_TYPE(probe_type), sum);
		if (sum.weight())
			return sum.sum() / sum.weight();
		else
			return nan("");
	}

	virtual std::vector<double>& Vars() {
		return m_vars;
	}

	~execution_engine() {
		for (size_t i = 0; i < m_vars.size(); i++)
			m_exec_param->m_vars[i].PopExecutionEngine();
	}
};

template<class value_type, class time_type> class lua_optimized_param_entry_in_buffer : public SzbParamObserver {
	base* m_base;
	TParam* m_param;

	std::vector<definable_param_cache<value_type, time_type> > m_cache;
public:
	lua_optimized_param_entry_in_buffer(base *_base, TParam* param, const boost::filesystem::wpath&) : m_base(_base), m_param(param)
	{
		for (SZARP_PROBE_TYPE p = PT_FIRST; p < PT_LAST; p = SZARP_PROBE_TYPE(p + 1))
			m_cache.push_back(definable_param_cache<value_type, time_type>(p));
	}

	void get_weighted_sum_impl(const time_type& start, const time_type& end, SZARP_PROBE_TYPE probe_type, weighted_sum<value_type, time_type>& sum)  {
		execution_engine ee(m_base, m_param);
		double v = ee.calculate_value(start, probe_type);
		if (!isnan(v)) {
			sum.sum() += v;
			sum.weight() += end - start;
		} else {
			sum.no_data_weight() += end - start;
		}
	}

	time_type search_data_right_impl(const time_type& start, const time_type& end, SZARP_PROBE_TYPE, const search_condition& condition) {
		return invalid_time_value<time_type>::value;
	}

	time_type search_data_left_impl(const time_type& start, const time_type& end, SZARP_PROBE_TYPE, const search_condition& condition) {
		return invalid_time_value<time_type>::value;
	}

	void register_at_monitor(SzbParamMonitor* monitor) {
	}

	void deregister_from_monitor(SzbParamMonitor* monitor) {
	}

	void param_data_changed(TParam*, const std::string& path) {
	}


};

}
#endif

