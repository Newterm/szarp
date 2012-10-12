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

template<class value_type, class time_type> class value_time_block {
public:
	typedef value_time_pair<value_type, time_type> value_time_type;
	typedef std::vector<value_time_type> value_time_vector;

	value_time_block(const time_type& time) : m_start_time(time) {}

	const time_type& start_time() const { return m_start_time; }
	const time_type end_time() const { return m_data[m_data.size() - 1].time; }

	typename value_time_vector::iterator search_entry_for_time(const time_type& time) {
		return sz4::search_entry_for_time(m_data.begin(), m_data.end(), time);
	}

	typename value_time_vector::const_iterator search_entry_for_time(const time_type& time) const {
		return sz4::search_entry_for_time(m_data.begin(), m_data.end(), time);
	}

	value_time_vector& data() {
		return m_data;
	}

	const value_time_vector& data() const {
		return m_data;
	}

	template<class search_op> value_time_vector::const_iterator search_data_right_t(const time_type& start, const time_type& end, const search_op &condition) {
		typename value_time_block<value_type, time_type>::value_time_vector::const_iterator i = search_entry_for_time(start);
		while (i != m_data.end()) {
			if (condition(i->value))
				return i;

			if (i->time >= end)
				break;
			std::advance(i, 1);
		}
		return m_data.end();
	}

	template<class search_op> value_time_vector::const_iterator search_data_left(const time_type& start, const time_type& end, const search_op &condition) {
		if (m_data.size() == 0)
			return m_data.end();

		typename value_time_block<value_type, time_type>::value_time_vector::const_iterator i = search_entry_for_time(start);
		if (i == m_data.end())
			std::advance(i, -1);

		while (true) {
			if (i->time <= end)
				break;

			if (condition(i->value))
				return i;

			if (i == m_data.begin())
				break;

			std::advance(i, -1);
		}
		return m_data.end();
	}


	void append_entry(const value_type& value, const time_type& time) {
		m_data.push_back(make_value_time_pair(value, time));
	}

	void append_entries(const value_time_vector& entries) {
		m_data.insert(m_data.end(), entries.begin(), entries.end());
	}

	void set_data(value_time_vector& data) { m_data.swap(data); }

	virtual ~value_time_block<value_type, time_type>() {}

protected:
	value_time_vector m_data;
	time_type m_start_time;
};

template<class value_type, class time_type> class concrete_block : public value_time_block<value_type, time_type> {
public:
	concrete_block(const time_type& start_time) : value_time_block<value_type, time_type>(start_time) {}
		
	void get_weighted_sum(const time_type& start_time, const time_type &end_time, weighted_sum<value_type, time_type>& r) const {
		time_type prev_time(start_time);

		bool done = false;
		typename value_time_block<value_type, time_type>::value_time_vector::const_iterator i = this->search_entry_for_time(start_time);
		while (!done && i != this->m_data.end()) {
			typename time_difference<time_type>::type time_diff;
			if (i->time >= end_time) {
				time_diff = end_time - prev_time;
				done = true;
			} else {
				time_diff = i->time - prev_time;
			}
			prev_time = i->time;

			if (!value_is_no_data(i->value)) {
				r.sum() += typename weighted_sum<value_type, time_type>::sum_type(i->value) * time_diff;
				r.weight() += time_diff;
			} else {
				r.no_data_weight() += time_diff;
			}

			std::advance(i, 1);
		}
	}

		
	time_type search_data_right(const time_type& start, const time_type& end, const search_condition &condition) {
		typename value_time_block<value_type, time_type>::value_time_vector::const_iterator i = this->search_data_right_t(start, end, condition);
		if (i == this->m_data.end())
			return invalid_time_value<time_type>::value;

		if (i == this->m_data.begin())
			return std::max(start, this->m_start_time);
		else 
			return std::max(i->time, start);
	}

	time_type search_data_left(const time_type& start, const time_type& end, const search_condition &condition) {
		typename value_time_block<value_type, time_type>::value_time_vector::const_iterator i = this->search_data_left_t(start, end, cond);
		if (i != this->m_data.end())
			return std::min(time_just_before<time_type>::get(i->time), start);
		else 
			return invalid_time_value<time_type>::value;
	}

};

}

#endif
