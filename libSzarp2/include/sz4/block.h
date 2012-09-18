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
#ifndef __SZ4_BLOCK_H__
#define __SZ4_BLOCK_H__

#include <algorithm>
#include <iterator>

#include "sz4/defs.h"
#include "sz4/time.h"

namespace sz4 {

template<class V, class T> struct value_time_pair<V, T> make_value_time_pair(V v, T t) {
	value_time_pair<V, T> p;
	p.value = v;
	p.time = t;
	return p;
}

template<class T, class V> bool cmp_time(const T& t, const value_time_pair<V, T>& p) {
	return t < p.time;
}

template<class I, class T> I search_entry_for_time(I begin, I end, const T& t) {
	typedef typename I::value_type pair_type;
	typedef typename pair_type::value_type value_type;

	return std::upper_bound(begin, end, t, cmp_time<T, value_type>);
}

template<class V, class T> class concrete_block {
public:
	typedef V value_type;
	typedef T time_type;
	typedef value_time_pair<V, T> value_time_type;
	typedef std::vector<value_time_type> value_time_vector;

	concrete_block(const time_type& start_time) : m_start_time(start_time) {}
		
	const time_type& start_time() const { return m_start_time; }
	const time_type end_time() const { return m_data[m_data.size() - 1].time; }

	void get_weighted_sum(const time_type& start_time, const time_type &end_time, weighted_sum<V, T>& r) const {
		time_type prev_time(start_time);

		bool done = false;
		typename value_time_vector::const_iterator i = search_entry_for_time(m_data.begin(), m_data.end(), start_time);
		while (!done && i != m_data.end()) {
			typename time_difference<time_type>::type time_diff;
			if (i->time >= end_time) {
				time_diff = end_time - prev_time;
				done = true;
			} else {
				time_diff = i->time - prev_time;
			}
			prev_time = i->time;

			if (!value_is_no_data(i->value)) {
				r.sum() += i->value * time_diff;
				r.weight() += time_diff;
			} else {
				r.no_data_weight() += time_diff;
			}

			std::advance(i, 1);
		}
	}

		
	T search_data_right(const T& start, const T& end, const search_condition &condition) {
		typename value_time_vector::const_iterator i = search_entry_for_time(m_data.begin(), m_data.end(), start);
		while (i != m_data.end()) {
			if (condition(i->value)) {
				if (i == m_data.begin())
					return std::max(start, m_start_time);
				else {
					std::advance(i, -1);
					return std::max(i->time, start);

				}
			}

			if (i->time >= end)
				break;
			std::advance(i, 1);
		}
		return invalid_time_value<T>::value;
	}

	T search_data_left(const T& start, const T& end, const search_condition &condition) {
		if (m_data.size() == 0)
			return invalid_time_value<T>::value;

		typename value_time_vector::const_iterator i = search_entry_for_time(m_data.begin(), m_data.end(), start);
		if (i == m_data.end())
			std::advance(i, -1);

		while (true) {
			if (i->time <= end)
				break;

			if (condition(i->value))
				return std::min(time_just_before<T>::get(i->time), start);

			if (i == m_data.begin())
				break;

			std::advance(i, -1);
		}
		return invalid_time_value<T>::value;
	}

	void set_data(value_time_vector& data) { m_data.swap(data); }
private:

	value_time_vector m_data;
	time_type m_start_time;
};

}

#endif
