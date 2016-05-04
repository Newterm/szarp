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

template<class value_type, class time_type>
void
live_block<value_type, time_type>::set_observer(live_values_observer* observer)
{
	m_observer.store(observer, std::memory_order_release);
}

template<class value_type, class time_type>
bool live_block<value_type, time_type>::get_weighted_sum(const time_type& start, time_type& end, weighted_sum<value_type, time_type>& sum)
{
	std::lock_guard<std::mutex> guard(m_lock);

	if (m_block.size() == 0)
		return false;

	sz4::get_weighted_sum(m_block.begin(), m_block.end(), m_start_time, end, sum);

	end = m_start_time;

	return start >= m_start_time;
}

template<class value_type, class time_type>
std::pair<bool, time_type> live_block<value_type, time_type>::search_data_left(const time_type& start, const time_type& end, const search_condition& condition)
{
	std::lock_guard<std::mutex> guard(m_lock);

	if (!m_block.size())
		return std::make_pair(false, end);

	if (end > m_block.back().time)
		return std::make_pair(false, time_trait<time_type>::invalid_value);

	if (start < m_start_time)
		return std::make_pair(false, start);

	auto i = sz4::search_data_left_t(m_block.begin(), m_block.end(), start, end, condition);
	if (i != m_block.end())
		return std::make_pair(true, std::min(time_just_before<time_type>::get(i->time), start));
	else 
		return std::make_pair(false, m_start_time);
}

template<class value_type, class time_type>
time_type live_block<value_type, time_type>::search_data_right(const time_type& start, const time_type& end, const search_condition& condition)
{
	std::lock_guard<std::mutex> guard(m_lock);

	if (!m_block.size())
		return time_trait<time_type>::invalid_value;

	auto i = sz4::search_data_left_t(m_block.begin(), m_block.end(), start, end, condition);
	if (i == m_block.end())
		return time_trait<time_type>::invalid_value;

	if (i == m_block.begin())
		return std::max(start, m_start_time);
	else
		return std::max(start, (i - 1)->time);
}

template<class value_type, class time_type>
void live_block<value_type, time_type>::get_first_time(time_type &t) { 
	std::lock_guard<std::mutex> guard(m_lock);

	if (m_block.size())
		t = m_start_time;
	else
		t = m_block.back().time;
}

template<class value_type, class time_type>
void live_block<value_type, time_type>::get_last_time(time_type &t) { 
	std::lock_guard<std::mutex> guard(m_lock);

	if (m_block.size())
		t = m_block.back().time;
	else
		t = time_trait<time_type>::invalid_value;
}

namespace {

void get_time(szarp::ParamValue* value, second_time_t &t) {
	t = value->time();
}

void get_time(szarp::ParamValue* value, nanosecond_time_t &t) {
	t = nanosecond_time_t(value->time(), value->nanotime());
}

void get_value(szarp::ParamValue* value, short& v) {
	v = value->int_value();
}

void get_value(szarp::ParamValue* value, int& v) {
	v = value->int_value();
}

void get_value(szarp::ParamValue* value, float& v) {
	v = value->float_value();
}

void get_value(szarp::ParamValue* value, double& v) {
	v = value->float_value();
}

}

template<class value_type, class time_type>
void live_block<value_type, time_type>::process_live_value(szarp::ParamValue* value)
{
	time_type t;
	get_time(value, t);

	value_type v;
	get_value(value, v);

	{
		std::lock_guard<std::mutex> guard(m_lock);
		bool add_new = !m_block.size() || m_block.back().value == v;
		if (add_new) {
			if (m_block.size() && (typename time_difference<time_type>::type(t - m_start_time)
											> m_retention)) {
				m_start_time = m_block.front().time;
				m_block.pop_front();
			}
			m_block.push_back(make_value_time_pair<value_time_pair<value_type, time_type>>(v, t));
		} else
			m_block.back().time = t;
	}

	auto observer = m_observer.load(std::memory_order_consume);
	if (observer)
		observer->new_live_value(value);

}

namespace {

void convert_retention(const time_difference<second_time_t>::type& f, time_difference<nanosecond_time_t>::type& t) {
	t = f;
	t *= 1000000000;

}

void convert_retention(const time_difference<second_time_t>::type& f, time_difference<second_time_t>::type& t) {
	t = f;
}

}

template<class value_type, class time_type>
live_block<value_type, time_type>::live_block(time_difference<second_time_t>::type retention) {
	convert_retention(retention, m_retention);
}

struct generic_live_block_builder {
	template<class data_type, class time_type, class... Args>
	static generic_live_block* op(Args... args) {
		return new live_block<data_type, time_type>(args...);
	}
};

template<class entry_builder> live_cache::live_cache(
		std::vector<std::pair<std::string, TSzarpConfig*>> configuration,
		time_difference<second_time_t>::type& cache_duration) {

	m_context = new zmq::context_t(1);

	for (auto& c : configuration) {
		m_urls.push_back(c.first);
		m_config_id_map.push_back(c.second->GetConfigId());

		auto cache = std::vector<generic_live_block*>();

		auto szarp_config = c.second();
		TParam* end = szarp_config->GetFirstDrawDefinable();
		for (TParam* p = szarp_config->GetFirstParam(); p != end;
				p = p->GetNext(true)) {

			auto entry(factory<generic_live_block, entry_builder>::op
				(p, cache_duration));

			cache.push_back(entry);
		}

		m_cache.push_back(std::move(cache));
	}
}

}

#endif
