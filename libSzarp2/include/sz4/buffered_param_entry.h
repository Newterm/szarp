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

template<class value_type, class time_type, class types, template<class types> class calculation_method> class buffered_param_entry_in_buffer : public SzbParamObserver {
protected:
	base_templ<types>* m_base;
	TParam* m_param;
	bool m_invalidate_non_fixed;

	typedef buffered_param_entry_in_buffer<value_type, time_type, types, calculation_method> _type;

	typedef definable_param_cache<value_type, time_type> cache_type;
	typedef std::vector<cache_type> cache_vector;
	cache_vector m_cache;
public:
	buffered_param_entry_in_buffer(base_templ<types>*_base, TParam* param, const boost::filesystem::wpath&) : m_base(_base), m_param(param), m_invalidate_non_fixed(false)
	{
		for (SZARP_PROBE_TYPE p = PT_FIRST; p < PT_LAST; p = SZARP_PROBE_TYPE(p + 1))
			m_cache.push_back(definable_param_cache<value_type, time_type>(p, _base->cache()));
	}

	void invalidate_non_fixed_if_needed() {
		if (!m_invalidate_non_fixed)
			return;

		m_invalidate_non_fixed = false;
		std::for_each(m_cache.begin(), m_cache.end(), std::mem_fun_ref(&cache_type::invalidate_non_fixed_values));
	}

	void adjust_time_range(time_type& from, time_type& to) {
		time_type start;
		m_base->get_first_time(this->m_param, start);
		if (!time_trait<time_type>::is_valid(start)) {
			to = from;
			return;
		}
	
		time_type end;
		m_base->get_last_time(this->m_param, end);
		if (!time_trait<time_type>::is_valid(end)) {
			to = from;
			return;
		} 

		from = std::max(start, from);
		to = std::min(end, to);
	}

	void get_weighted_sum_impl(time_type start, time_type end, SZARP_PROBE_TYPE probe_type, weighted_sum<value_type, time_type>& sum)  {
		invalidate_non_fixed_if_needed();

		calculation_method<types> ee(m_base, m_param);

		time_type range_end;
		auto read_ahead(m_base->read_ahead());
		if (read_ahead)
			range_end = std::max<time_type>(szb_move_time(start, 1, read_ahead.get()), end);
		else
			range_end = end;
		
		SZARP_PROBE_TYPE step = get_probe_type_step(probe_type);
		time_type from = start, to = range_end;

		adjust_time_range(from, to);

		if (!read_ahead)
			m_base->read_ahead() = probe_type;
		else if (!(start == from))
			m_base->read_ahead() = boost::optional<SZARP_PROBE_TYPE>();

		sum.add_no_data_weight(from - start);
		if (to < end) {
			sum.add_no_data_weight(end - to);
			sum.set_fixed(false);
		}

		bool first = true;
		time_type& current(start);
		while (current < to) {
			value_type value;
			bool fixed;
			generic_block_ptr_set refferred_blocks;
			time_type next = szb_move_time(current, 1, step);

			if (!m_cache[step].get_value(current,
						value,
						fixed,
						refferred_blocks)) {
				std::tr1::tie(value, fixed) = ee.calculate_value(current, step, refferred_blocks);
				if (current < end) {
					if (!value_is_no_data(value))
						sum.add(value, next - current);
					else
						sum.add_no_data_weight(next - current);
					sum.add_refferred_blocks(
							refferred_blocks.begin(),
							refferred_blocks.end()
							);
					sum.set_fixed(fixed);
				}

				m_cache[step].store_value(value, current, fixed, std::move(refferred_blocks));
			}

			if (first) {
				first = false;
				m_base->read_ahead() = boost::optional<SZARP_PROBE_TYPE>();
			}

			current = next;
		}

		m_base->read_ahead() = read_ahead;
	}

	time_type search_data_right_impl(time_type start, time_type end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		invalidate_non_fixed_if_needed();

		calculation_method<types> ee(m_base, m_param);

		adjust_time_range(start, end);
		
		time_type& current(start);

		while (current < end) {
			std::pair<bool, time_type> r = m_cache[probe_type].search_data_right(current, end, condition);
			if (r.first)
				return r.second;
			
			current = r.second;

			generic_block_ptr_set block_set;
			std::tr1::tuple<double,
				bool>
				vf(ee.calculate_value(current, probe_type, block_set));
			m_cache[probe_type].store_value(
					std::tr1::get<0>(vf),
					current,
					std::tr1::get<1>(vf),
					std::move(block_set));	

		}

		return time_trait<time_type>::invalid_value;
	}

	time_type search_data_left_impl(time_type start, time_type end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		invalidate_non_fixed_if_needed();

		calculation_method<types> ee(m_base, m_param);
		
		adjust_time_range(end, start);

		time_type current(start);

		while (current > end) {
			std::pair<bool, time_type> r = m_cache[probe_type].search_data_left(current, end, condition);
			if (r.first)
				return r.second;
			current = r.second;

			generic_block_ptr_set block_set;
			std::tr1::tuple<double, bool>
				 vf(ee.calculate_value(current, probe_type, block_set));
			m_cache[probe_type].store_value(std::tr1::get<0>(vf),
					current,
					std::tr1::get<1>(vf),
					std::move(block_set));
		}

		return time_trait<time_type>::invalid_value;
	}

	void get_first_time(std::list<generic_param_entry*>& referred_params, time_type &t) {
		t = time_trait<time_type>::invalid_value;

		if (!referred_params.size())
			return m_base->get_heartbeat_first_time(m_param, t);

		for (std::list<generic_param_entry*>::const_iterator i = referred_params.begin();
				i != referred_params.end();
				i++) {
			time_type t1;
			(*i)->get_first_time(t1);

			if (!time_trait<time_type>::is_valid(t1)) {
				t = time_trait<time_type>::invalid_value;
				return;
			}

			if (time_trait<time_type>::is_valid(t))
				t = std::max(t1, t);
			else
				t = t1;
		}

	}

	void get_last_time(const std::list<generic_param_entry*>& referred_params, time_type &t) {
		t = time_trait<time_type>::invalid_value;

		if (!referred_params.size())
			return m_base->get_heartbeat_last_time(m_param, t);

		for (std::list<generic_param_entry*>::const_iterator i = referred_params.begin();
				i != referred_params.end();
				i++) {
			time_type t1;
			(*i)->get_last_time(t1);

			if (!time_trait<time_type>::is_valid(t1)) {
				t = time_trait<time_type>::invalid_value;
				return;
			}

			if (time_trait<time_type>::is_valid(t))
				t = std::min(t1, t);
			else
				t = t1;
		}

	}

	void register_at_monitor(generic_param_entry* entry, SzbParamMonitor* monitor) {
	}

	void deregister_from_monitor(generic_param_entry* entry, SzbParamMonitor* monitor) {
	}

	void param_data_changed(TParam*, const std::string& path) {
		m_invalidate_non_fixed = true;
	}

	void refferred_param_removed(generic_param_entry* param_entry) {
		delete m_param->GetLuaExecParam();
		m_param->SetLuaExecParam(NULL);
		//go trough preparation procedure again
		//XXX: reset cache as well
		m_param->SetSz4Type(TParam::SZ4_NONE);
		generic_param_entry::refferred_param_removed(param_entry);
	}

	virtual ~buffered_param_entry_in_buffer() {};
};

}

#endif
