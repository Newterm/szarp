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
#ifndef __SZ4_PARAM_ENTRY_H__
#define __SZ4_PARAM_ENTRY_H__

#include "config.h"

#include "live_observer.h"

namespace szarp {
class ParamValue;
}

namespace sz4 {

class generic_live_block;

class generic_param_entry : public SzbParamObserver, public live_values_observer {
protected:
	boost::recursive_mutex m_lock;
	TParam* m_param;

	std::list<generic_param_entry*> m_referring_params;
	std::list<generic_param_entry*> m_referred_params;
	std::list<param_observer*> m_observers;
public:
	generic_param_entry(TParam* param) : m_param(param) {}
	TParam* get_param() const { return m_param; }

	virtual void get_weighted_sum(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<short, second_time_t>& wsum) = 0;

	virtual void get_weighted_sum(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<int, second_time_t>& wsum) = 0;

	virtual void get_weighted_sum(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<float, second_time_t>& wsum) = 0;

	virtual void get_weighted_sum(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<double, second_time_t>& wsum) = 0;

	virtual void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<short, nanosecond_time_t>& wsum) = 0;

	virtual void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<int, nanosecond_time_t>& wsum) = 0;

	virtual void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<float, nanosecond_time_t>& wsum) = 0;

	virtual void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<double, nanosecond_time_t>& wsum) = 0;

	virtual second_time_t search_data_right(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) = 0;

	virtual second_time_t search_data_left(const second_time_t& start, const second_time_t& end,  SZARP_PROBE_TYPE probe_type,const search_condition& condition) = 0;

	virtual nanosecond_time_t search_data_right(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) = 0;

	virtual nanosecond_time_t search_data_left(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) = 0;

	virtual void get_first_time(nanosecond_time_t& t) = 0;
	virtual void get_first_time(second_time_t& t) = 0;

	virtual void get_last_time(nanosecond_time_t& t) = 0;
	virtual void get_last_time(second_time_t& t) = 0;

	virtual void register_at_monitor(SzbParamMonitor* monitor) = 0;
	virtual void deregister_from_monitor(SzbParamMonitor* monitor) = 0;
	void param_data_changed(TParam*, const std::string& path);
	virtual void handle_param_data_changed(TParam*, const std::string& path) {};

	const std::list<generic_param_entry*>& referring_params() const;
	const std::list<generic_param_entry*>& referred_params() const;

	void add_refferring_param(generic_param_entry* param_entry);
	void remove_refferring_param(generic_param_entry* param_entry);

	void add_refferred_param(generic_param_entry *param_entry);
	virtual void refferred_param_removed(generic_param_entry* param_entry);

	void register_observer(param_observer *observer);
	void deregister_observer(param_observer *observer);

	virtual void new_live_value(szarp::ParamValue *value);

	virtual void set_live_block(generic_live_block *block) = 0;

	virtual ~generic_param_entry();
};

namespace param_buffer_type_converion_helper {

template<class IV, class IT, class OV, class OT> class helper {
	weighted_sum<IV, IT> m_temp;
	weighted_sum<OV, OT>& m_sum;
public:
	helper(weighted_sum<OV, OT>& sum) : m_sum(sum) {}
	weighted_sum<IV, IT>& sum() { return m_temp; }
	void convert() {
		m_sum.take_from(m_temp);
	}
};

template<class V, class T> class helper<V, T, V, T> {
	weighted_sum<V, T>& m_sum;
public:
	helper(weighted_sum<V, T>& sum) : m_sum(sum) {}
	weighted_sum<V, T>& sum() { return m_sum; }
	void convert() { }
};

}

template<template <typename DT, typename TT, typename BT> class PT, class V, class T, class BT> class param_entry_in_buffer : public generic_param_entry {
	PT<V, T, BT> m_entry;
	
	template<class RV, class RT> void get_weighted_sum_templ(const T& start, const T& end, SZARP_PROBE_TYPE probe_type, weighted_sum<RV, RT>& sum)  {
		param_buffer_type_converion_helper::helper<V, T, RV, RT> helper(sum);
		m_entry.get_weighted_sum_impl(start, end, probe_type, helper.sum());
		helper.convert();
	}
public:
	param_entry_in_buffer(BT* _base, TParam* param, const boost::filesystem::wpath& base_dir) :
		generic_param_entry(param), m_entry(_base, param, base_dir / param->GetSzbaseName())
	{ }

	void get_weighted_sum(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<short, second_time_t>& wsum) {
		probe_adapter<second_time_t, T>()(probe_type);
		get_weighted_sum_templ(T(start), round_up<second_time_t, T>()(end), probe_type, wsum);
	}

	void get_weighted_sum(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<int, second_time_t>& wsum) {
		probe_adapter<second_time_t, T>()(probe_type);
		get_weighted_sum_templ(T(start), round_up<second_time_t, T>()(end), probe_type, wsum);
	}

	void get_weighted_sum(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<float, second_time_t>& wsum) {
		probe_adapter<second_time_t, T>()(probe_type);
		get_weighted_sum_templ(T(start), round_up<second_time_t, T>()(end), probe_type, wsum);
	}

	void get_weighted_sum(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<double, second_time_t>& wsum) {
		probe_adapter<second_time_t, T>()(probe_type);
		get_weighted_sum_templ(T(start), round_up<second_time_t, T>()(end), probe_type, wsum);
	}

	void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<short, nanosecond_time_t>& wsum) {
		probe_adapter<nanosecond_time_t, T>()(probe_type);
		get_weighted_sum_templ(T(start), round_up<nanosecond_time_t, T>()(end), probe_type, wsum);
	}

	void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<int, nanosecond_time_t>& wsum) {
		probe_adapter<nanosecond_time_t, T>()(probe_type);
		get_weighted_sum_templ(T(start), round_up<nanosecond_time_t, T>()(end), probe_type, wsum);
	}

	void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<float, nanosecond_time_t>& wsum) {
		probe_adapter<nanosecond_time_t, T>()(probe_type);
		get_weighted_sum_templ(T(start), round_up<nanosecond_time_t, T>()(end), probe_type, wsum);
	}

	void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, weighted_sum<double, nanosecond_time_t>& wsum) {
		probe_adapter<nanosecond_time_t, T>()(probe_type);
		get_weighted_sum_templ(T(start), round_up<nanosecond_time_t, T>()(end), probe_type, wsum);
	}

	second_time_t search_data_right(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return search_data_right_templ(start, end, probe_type, condition);
	}

	second_time_t search_data_left(const second_time_t& start, const second_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return search_data_left_templ(start, end, probe_type, condition);
	}

	nanosecond_time_t search_data_right(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return search_data_right_templ(start, end, probe_type, condition);
	}

	nanosecond_time_t search_data_left(const nanosecond_time_t& start, const nanosecond_time_t& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return search_data_left_templ(start, end, probe_type, condition);
	}

	template<class RT> RT search_data_right_templ(const RT& start, const RT &end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		probe_adapter<RT, T>()(probe_type);
		return RT(m_entry.search_data_right_impl(T(start), round_up<RT, T>()(end), probe_type, condition));
	}

	template<class RT> RT search_data_left_templ(const RT& start, const RT &end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		probe_adapter<RT, T>()(probe_type);
		return RT(m_entry.search_data_left_impl(T(start), round_up<RT, T>()(end), probe_type, condition));
	}

	template<class RT> void get_first_time_templ(RT& t) {
		T tt;
		m_entry.get_first_time(m_referred_params, tt);
		t = RT(tt);
	}

	void get_first_time(nanosecond_time_t& t) {
		get_first_time_templ(t);
	}

	void get_first_time(second_time_t& t) {
		get_first_time_templ(t);
	}

	template<class RT> void get_last_time_templ(RT& t) {
		T tt;
		m_entry.get_last_time(m_referred_params, tt);
		t = RT(tt);
	}

	void get_last_time(nanosecond_time_t& t) {
		get_last_time_templ(t);
	}	

	void get_last_time(second_time_t& t) {
		get_last_time_templ(t);
	}

	void register_at_monitor(SzbParamMonitor* monitor) {
		m_entry.register_at_monitor(this, monitor);
	}

	void deregister_from_monitor(SzbParamMonitor* monitor) {
		m_entry.deregister_from_monitor(this, monitor);
	}

	void handle_param_data_changed(TParam* param, const std::string& path) {
		m_entry.param_data_changed(param, path);
	}
			
	PT<V, T, BT>& get_contained_entry() {
		return m_entry;
	}

	void set_live_block(generic_live_block *block) {
		m_entry.set_live_block(block);
	}

	virtual ~param_entry_in_buffer() {
	}
};

}

#endif
