#ifndef __SZ4_API_H__
#define __SZ4_API_H__

#include "szarp_config.h"
#include "sz4/param_observer.h"

namespace sz4 {

template<class I> class sz4_api {
	I* m_impl;
public:
	sz4_api(I *impl);

	template<class V, class T> void get_weighted_sum(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, std::vector< weighted_sum<V, T> >&) > cb) {
		m_impl->get_weighted_sum(param, start, end, probe_type, cb);
	}

	template<class T> void search_data_right(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(const boost::system::error_code&, T&) > cb) {
		m_impl->search_data_right(param, start, end, probe_type, condition, cb);
	}

	template<class T> void search_data_left(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(const boost::system::error_code&, T&) > cb) {
		m_impl->search_data_left(param, start, end, probe_type, condition, cb);
	}

	template<class T> void get_first_time(TParam* param, std::function<void(const boost::system::error_code&, T&) > cb) {
		m_impl->get_first_time(param, cb);
	}

	template<class T> void get_last_time(TParam* param, std::function<void(const boost::system::error_code&, T&) > cb) {
		m_impl->get_last_time(param, cb);
	}

	void register_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(const boost::system::error_code&) > cb) {
		m_impl->register_observer(observer, params, cb);
	}

	void deregister_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(const boost::system::error_code&) > cb) {
		m_impl->deregister_observer(observer, params, cb);
	}

	void add_param(TParam* param, std::function<void(const boost::system::error_code&) > cb) {
		m_impl->add_param(param, cb);
	}

	void remove_param(TParam* param, std::function<void(const boost::system::error_code&) > cb) {
		m_impl->remove_param(param, cb);
	}
};

}

#endif
