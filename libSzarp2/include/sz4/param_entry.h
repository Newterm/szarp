#ifndef __SZ4_PARAM_ENTRY_H__
#define __SZ4_PARAM_ENTRY_H__
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

#include <boost/preprocessor/seq/for_each_product.hpp>
#include <boost/preprocessor/seq/elem.hpp>

namespace sz4 {

template<class types> class base_templ;

#define SZ4_BASE_DATA_TYPE_SEQ (short)(int)(float)(double)
#define SZ4_BASE_TIME_TYPE_SEQ (second_time_t)(nanosecond_time_t)

class generic_param_entry : public SzbParamObserver {
protected:
	boost::recursive_mutex m_lock;
	TParam* m_param;

	std::list<generic_param_entry*> m_referring_params;
	std::list<generic_param_entry*> m_referred_params;
	std::list<param_observer*> m_observers;
public:
	generic_param_entry(TParam* param) : m_param(param) {}
	TParam* get_param() const { return m_param; }

	/* Macro below generates methods
	 virtual void get_weighted_sum(time_type start_time, time_type end_type, weighted_sum<time_type, value_type>() = 0;
	 for each possible combination of value and time types
	*/
#	define SZ4_GENERATE_ABSTRACT_GET_WSUM(r, seq) 	\
	virtual void get_weighted_sum(const BOOST_PP_SEQ_ELEM(1, seq)& start, const BOOST_PP_SEQ_ELEM(1, seq)& end, SZARP_PROBE_TYPE probe_type, weighted_sum<BOOST_PP_SEQ_ELEM(0, seq), BOOST_PP_SEQ_ELEM(1, seq)>& wsum) = 0;

	BOOST_PP_SEQ_FOR_EACH_PRODUCT_R(1, SZ4_GENERATE_ABSTRACT_GET_WSUM, (SZ4_BASE_DATA_TYPE_SEQ)(SZ4_BASE_TIME_TYPE_SEQ))

#	undef SZ4_GENERATE_ABSTRACT_GET_WSUM
	

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

	virtual ~generic_param_entry();
};

template<class V, class T> class block_entry {
protected:
	concrete_block<V, T> m_block;
	bool m_needs_refresh;
	const boost::filesystem::wpath m_block_path;
public:
	block_entry(const T& start_time, block_cache* cache) :
		m_block(start_time, cache), m_needs_refresh(true) {}

	void get_weighted_sum(const T& start, const T& end, weighted_sum<V, T>& wsum) {
		m_block.get_weighted_sum(start, end, wsum);
	}

	T search_data_right(const T& start, const T& end, const search_condition& condition) {
		return m_block.search_data_right(start, end, condition);
	}

	T search_data_left(const T& start, const T& end, const search_condition& condition) {
		return m_block.search_data_left(start, end, condition);
	}

	void set_needs_refresh() {
		m_needs_refresh = true;
	}

	concrete_block<V, T>& block() { return m_block; }
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
	param_entry_in_buffer(base_templ<BT>* _base, TParam* param, const boost::filesystem::wpath& base_dir) :
		generic_param_entry(param), m_entry(_base, param, base_dir / param->GetSzbaseName())
	{ }

#	define SZ4_GENERATE_GET_WSUM(r, seq) 	\
	void get_weighted_sum(const BOOST_PP_SEQ_ELEM(1, seq)& start, const BOOST_PP_SEQ_ELEM(1, seq)& end, SZARP_PROBE_TYPE probe_type, weighted_sum<BOOST_PP_SEQ_ELEM(0, seq), BOOST_PP_SEQ_ELEM(1, seq)>& wsum) {	  \
		probe_adapter<BOOST_PP_SEQ_ELEM(1, seq), T>()(probe_type); \
		get_weighted_sum_templ(start, round_up<BOOST_PP_SEQ_ELEM(1, seq), T>()(end), probe_type, wsum);	\
	}

	BOOST_PP_SEQ_FOR_EACH_PRODUCT_R(1, SZ4_GENERATE_GET_WSUM, (SZ4_BASE_DATA_TYPE_SEQ)(SZ4_BASE_TIME_TYPE_SEQ))
#	undef SZ4_GENERATE_GET_WSUM

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
		t = tt;
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
		t = tt;
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
			
	PT<V, T, base_templ<BT> >& get_contained_entry() {
		return m_entry;
	}

	virtual ~param_entry_in_buffer() {
	}
};

}

#endif
