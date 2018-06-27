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

#ifndef MINGW32
#include <sys/types.h>
#include <sys/socket.h>
#endif

namespace sz4 {

template<class value_type, class time_type>
void
live_block<value_type, time_type>::set_observer(live_values_observer* observer)
{
	m_observer.store(observer, std::memory_order_release);
}

template<class value_type, class time_type>
cache_ret live_block<value_type, time_type>::get_weighted_sum(const time_type& start, time_type& end, weighted_sum<value_type, time_type>& sum)
{
	std::lock_guard<std::mutex> guard(m_lock);

	if (m_block.size() == 0 || end <= m_start_time)
		return cache_ret::none;

	if (start >= m_block.back().time) {
		sum.add_no_data_weight(end - start);
		sum.set_fixed(false);
		return cache_ret::complete;
	}

	sz4::get_weighted_sum(m_block.begin(), m_block.end(),
				std::max(start, m_start_time),
				end, sum);

	if (end > m_block.back().time) {
		sum.add_no_data_weight(end - m_block.back().time);
		sum.set_fixed(false);
	}

	end = m_start_time;

	return start >= m_start_time ? cache_ret::complete : cache_ret::partial;
}

template<class value_type, class time_type>
std::pair<bool, time_type> live_block<value_type, time_type>::search_data_left(const time_type& start, const time_type& end, const search_condition& condition)
{
	std::lock_guard<std::mutex> guard(m_lock);

	if (!m_block.size() || (m_block.size() == 1 && m_start_time == m_block[0].time))
		return std::make_pair(false, start);

	if (end >= m_block.back().time)
		return std::make_pair(true, time_trait<time_type>::invalid_value);

	if (start < m_start_time)
		return std::make_pair(false, start);

	auto i = sz4::search_data_left_t(m_block.begin(), m_block.end(), start, end, condition);
	if (i != m_block.end())
		return std::make_pair(true, std::min(time_just_before(i->time), start));
	else 
		return std::make_pair(false, m_start_time);
}

template<class value_type, class time_type>
time_type live_block<value_type, time_type>::search_data_right(const time_type& start, const time_type& end, const search_condition& condition)
{
	std::lock_guard<std::mutex> guard(m_lock);

	if (!m_block.size() || end <= m_start_time || (m_block.size() == 1 && m_start_time == m_block[0].time))
		return time_trait<time_type>::invalid_value;

	auto i = sz4::search_data_right_t(m_block.begin(), m_block.end(), start, end, condition);
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
}

template<class value_type, class time_type>
void live_block<value_type, time_type>::get_last_time(time_type &t) { 
	std::lock_guard<std::mutex> guard(m_lock);

	if (m_block.size())
		t = m_block.back().time;
	else
		t = time_trait<time_type>::invalid_value;
}

#ifndef MINGW32
namespace {

void get_time(szarp::ParamValue* value, second_time_t &t) {
	t = value->time();
}

void get_time(szarp::ParamValue* value, nanosecond_time_t &t) {
	t = nanosecond_time_t(value->time(), value->nanotime());
}

void get_value(szarp::ParamValue* value, int16_t& v) {
	v = value->int_value();
}

void get_value(szarp::ParamValue* value, int32_t& v) {
	v = value->int_value();
}

void get_value(szarp::ParamValue* value, float& v) {
	v = value->double_value();
}

void get_value(szarp::ParamValue* value, double& v) {
	v = value->double_value();
}

void get_value(szarp::ParamValue* value, uint16_t& v) {
	v = value->int_value();
}

void get_value(szarp::ParamValue* value, uint32_t& v) {
	v = value->int_value();
}

}
#endif

template<class value_type, class time_type>
void live_block<value_type, time_type>::process_live_value(
		const time_type& t,
		const value_type& v)
{
	typedef typename time_difference<time_type>::type diff_type;
	std::lock_guard<std::mutex> guard(m_lock);

	if (m_block.size() && m_block.back().value == v) {
		m_block.back().time = t;
		return;
	}

	typedef value_time_pair<value_type, time_type> pair;
	if (!m_block.size()) {
		m_start_time = t;
		m_block.push_back(make_value_time_pair<pair>(v, t));
		return;
	}


	while (m_block.size() && diff_type(t - m_block[0].time) >= m_retention) {
		m_start_time = m_block.front().time;
		m_block.pop_front();
	}

	m_block.push_back(make_value_time_pair<pair>(v, t));
}

template<class value_type, class time_type>
void live_block<value_type, time_type>::process_live_value(szarp::ParamValue* value)
{
#ifndef MINGW32
	time_type t;
	get_time(value, t);

	value_type v;
	get_value(value, v);

	process_live_value(t, v);

	auto observer = m_observer.load(std::memory_order_consume);
	if (observer)
		observer->new_live_value(value);
#endif
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
live_block<value_type, time_type>::live_block(time_difference<second_time_t>::type retention) : m_observer(nullptr) {
	convert_retention(retention, m_retention);
}

struct generic_live_block_builder {
	template<class data_type, class time_type, class... Args>
	static generic_live_block* op(Args... args) {
		return new live_block<data_type, time_type>(args...);
	}
};

template<class entry_builder> live_cache::live_cache(
	const live_cache_config& config,
	zmq::context_t* context)
#ifndef MINGW32
	:
	m_context(context)
#endif
{
#ifndef MINGW32
	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, m_cmd_sock))
		throw std::runtime_error("Failed to craete socketpair");

	for (auto& c : config.urls) {
		m_urls.push_back(c.first);
		unsigned id = c.second->GetConfigId();

		if (m_cache.size() <= id)
			m_cache.resize(id + 1);

		m_sock_map.push_back(id);

		std::vector<generic_live_block*> config_cache;

		auto szarp_config = c.second;
		auto end = szarp_config->GetFirstDrawDefinable();

		for (auto p = szarp_config->GetFirstParam();
				p != end;
				p = p->GetNextGlobal()) {

			auto entry(factory<generic_live_block, entry_builder>::op
				(p, config.retention));

			config_cache.push_back(entry);
		}

		m_cache[id] = std::move(config_cache);
	}

	start();
#endif
}


}

#endif
