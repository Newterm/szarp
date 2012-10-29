#ifndef __BUFFERED_PARAM_ENTRY_H__
#define __BUFFERED_PARAM_ENTRY_H__
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

namespace sz4 {

template<class value_type, class time_type, class types, class calculation_method> class buffered_param_entry_in_buffer : public SzbParamObserver {
	base_templ<types>* m_base;
	TParam* m_param;
	bool m_invalidate_non_fixed;

	typedef definable_param_cache<value_type, time_type> cache_type;
	typedef std::vector<cache_type> cache_vector;
	cache_vector m_cache;
public:
	buffered_param_entry_in_buffer(base_templ<types>*_base, TParam* param, const boost::filesystem::wpath&) : m_base(_base), m_param(param), m_invalidate_non_fixed(false)
	{
		for (SZARP_PROBE_TYPE p = PT_FIRST; p < PT_LAST; p = SZARP_PROBE_TYPE(p + 1))
			m_cache.push_back(definable_param_cache<value_type, time_type>(p));
	}

	void invalidate_non_fixed_if_needed() {
		if (!m_invalidate_non_fixed)
			return;

		m_invalidate_non_fixed = true;
		std::for_each(m_cache.begin(), m_cache.end(), std::mem_fun_ref(&cache_type::invalidate_non_fixed_values));
	}

	void get_weighted_sum_impl(const time_type& start, const time_type& end, SZARP_PROBE_TYPE probe_type, weighted_sum<value_type, time_type>& sum)  {
		invalidate_non_fixed_if_needed();

		time_type current(start);
		caluculation_method<types> ee(m_base, m_param);

		while (current < end) {
			value_type value;
			bool fixed;
			if (m_cache[probe_type].get_value(current, value, fixed) == false) {
				std::pair<double, bool> r = ee.calculate_value(current, probe_type);
				value = r.first;
				fixed = r.second;
				m_cache[probe_type].store_value(value, current, fixed);
			}

			time_type next = szb_move_time(current, 1, probe_type);
			if (!value_is_no_data(value)) {
				sum.sum() += value * (next - current);
				sum.weight() += next - current;
			} else
				sum.no_data_weight() += next - current;
			sum.set_fixed(fixed);
			current = next;
		}

	}

	time_type search_data_right_impl(const time_type& start, const time_type& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		invalidate_non_fixed_if_needed();

		caluculation_method<types> ee(m_base, m_param);
		time_type current(start);
		while (true) {
			std::pair<bool, time_type> r = m_cache[probe_type].search_data_right(current, end, condition);
			if (r.first)
				return r.second;
			
			current = r.second;
			std::pair<double, bool> vf = ee.calculate_value(current, probe_type);
			m_cache[probe_type].store_value(vf.first, current, vf.second);
		}

		return invalid_time_value<time_type>::value;
	}

	time_type search_data_left_impl(const time_type& start, const time_type& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		invalidate_non_fixed_if_needed();

		caluculation_method<types> ee(m_base, m_param);
		time_type current(start);
		while (true) {
			std::pair<bool, time_type> r = m_cache[probe_type].search_data_left(current, end, condition);
			if (r.first)
				return r.second;
			current = r.second;

			std::pair<double, bool> vf = ee.calculate_value(current, probe_type);
			m_cache[probe_type].store_value(vf.first, current, vf.second);
		}

		return invalid_time_value<time_type>::value;
	}

	time_type get_first_time() {
		return invalid_time_value<time_type>::value;
	}

	time_type get_last_time() {
		return invalid_time_value<time_type>::value;
	}

	void register_at_monitor(generic_param_entry* entry, SzbParamMonitor* monitor) {
	}

	void deregister_from_monitor(generic_param_entry* entry, SzbParamMonitor* monitor) {
	}

	void param_data_changed(TParam*, const std::string& path) {
		m_invalidate_non_fixed = true;
	}

	void reffered_param_removed(generic_param_entry* param_entry) {
		delete m_param->GetLuaExecParam();
		m_param->SetLuaExecParam(NULL);
		//go trough preparation procedure again
		//XXX: reset cache as well
		m_param->SetSz4Type(TParam::SZ4_NONE);
	}

	virtual ~buffered_param_entry_in_buffer();

};

}

#endif
