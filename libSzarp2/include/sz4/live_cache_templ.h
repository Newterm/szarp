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
#ifndef __SZ4_LIVE_CACHE_TEMPL_H__
#define __SZ4_LIVE_CACHE_TEMPL_H__

namespace sz4 {

template<class value_type>
second_time_t
live_block<value_type, second_time_t>::get_time(szarp::ParamValue* value)
{
	return value->time();
}

template<class value_type>
nanosecond_time_t
live_block<value_type, nanosecond_time_t>::get_time(szarp::ParamValue* value)
{
	return nanosecond_time_t(value->time(), value->nanotime());
}

template<class time_type>
short
live_block<short, time_type>::get_value(szarp::ParamValue* value)
{
	return value->int_value();
}

template<class time_type>
int
live_block<int, time_type>::get_value(szarp::ParamValue* value)
{
	return value->int_value();
}

template<class time_type>
float
live_block<float, time_type>::get_value(szarp::ParamValue* value)
{
	return value->float_value();
}

template<class time_type>
double
live_block<double, time_type>::get_value(szarp::ParamValue* value)
{
	return value->double_value();
}

template<class value_type, class time_type>
void
live_block<value_type, time_type>::set_observer(live_values_observer* observer)
{
	m_observer.store(observer, std::memory_order_release);
}

template<class value_type, class time_type>
void live_block<value_type, time_type>::process_live_value(szarp::ParamValue* value)
{
	auto t = get_time(value);
	auto v = get_value(value);

	{
		std::lock_guard<std::mutex> guard(m_lock);
		bool add_new = !m_block.size() || m_block.back().second == v;
		if (add_new) {
			if (m_block.size() && (t - m_start_time > m_retention)) {
				m_start_time = m_block.first.second;
				m_block.pop_front();
			}
			m_block.push_back(std::make_pair(v, t));
		} else
			m_block.back().second = t;
	}

	auto observer = m_observer.load(std::memory_order_consume);
	if (observer)
		observer->new_live_value(value);

}

template<class value_type, class sz4::nanosecond_time_t>
live_block<value_type, time_type>::live_block(time_difference<second_time_t>::type retention) {
	m_retention = retention;
	m_retention *= 1000000000;
}

template<class value_type, class sz4::nanosecond_time_t>
live_block<value_type, time_type>::live_block(time_difference<second_time_t>::type retention) {
	m_retention = retention;
}

struct generic_live_entry_builder {
	template<class data_type, class time_type, Args...>
	static generic_live_entry* op(Args... args) {
		return new live_block<data_type, time_type>(args...);
	}
};

template<class entry_builder> live_cache<entry_builderk>::live_cache(
		std::vector<std::pair<std::string, TSzarpConfig*>> configuration,
		sz4::time_traits<second_time_t>::time_diff& cache_duration) {

	m_zmq_ctx = new zmq::context_t(1);

	for (auto& c : configuration) {
		m_urls.push_back(c.first);
		m_config_id_map.push_back(c.second->GetConfigId());

		auto cache = new std::vector<generic_live_entry>();

		auto szarp_config = c.second();
		TParam* end = szarp_config->GetFirstDrawDefinable();
		for (TParam* param = szarp_config->GetFirstParam(); p != end;
				p = p->GetNext(true)) {

			auto entry(factory<generic_live_entry, entry_builder>::op
				(cache_duration);

			cache->push_back(entry);
		}

		m_cache.push_back(cache);
	}
}
#endif
