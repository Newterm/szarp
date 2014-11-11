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
#ifndef __SZ4_DEFS_H__
#define __SZ4_DEFS_H__

#include <limits>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <numeric>
#include <vector>
#include <stdint.h>

#include <tr1/functional>

#include <boost/pool/pool_alloc.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/interprocess/sync/null_mutex.hpp>

#include "sz4/time.h"

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 \
					 + __GNUC_MINOR__ * 100 \
					 + __GNUC_PATCHLEVEL__)
#endif

namespace sz4 {

template<class V, class T> struct value_time_pair {
	typedef V value_type;
	typedef T time_type;

	value_type value;
	time_type  time;
} __attribute__ ((packed));

template<class T> bool value_is_no_data(const T& v) {
	return v == std::numeric_limits<T>::min();
}

bool value_is_no_data(const double& v);

bool value_is_no_data(const float& v);

template<class T> T no_data(){ 
	return std::numeric_limits<T>::min();
}

template<> float no_data<float>();

template<> double no_data<double>();

template<class T> struct time_difference { };

template<> struct time_difference<second_time_t> {
	typedef long type;
};

template<> struct time_difference<nanosecond_time_t> {
	typedef long long type;
};

template<class T> struct value_sum { };

template<> struct value_sum<short> {
	typedef long long type;
};

template<> struct value_sum<int> {
	typedef long long type;
};

template<> struct value_sum<unsigned int> {
	typedef unsigned long long type;
};

template<> struct value_sum<float> {
	typedef double type;
};

template<> struct value_sum<double> {
	typedef double type;
};

class generic_block;


template<class T> class allocator_type : public
	//boost::pool_allocator<T, boost::default_user_allocator_new_delete, boost::interprocess::null_mutex>
	std::allocator<T>
{
#if GCC_VERSION >= 40800
	public:
		using std::allocator<T>::allocator;
#endif
};

typedef boost::container::flat_set<generic_block*, std::less<generic_block*>, allocator_type<generic_block*> >
	generic_block_ptr_set;

template<class value_type, class time_type> class weighted_sum {
public:
	typedef typename value_sum<value_type>::type sum_type;
	typedef typename time_difference<time_type>::type time_diff_type;

	typedef std::vector<value_type, allocator_type<value_type> > value_vector;
	typedef std::vector<time_diff_type, allocator_type<time_diff_type> > time_diff_vector;
private:
	value_vector m_values;
	time_diff_vector m_weights;
	time_diff_type m_gcd;

	time_diff_type m_no_data_weight;
	bool m_fixed;

	value_type m_one_value;
	time_diff_type m_one_weight;
	bool m_has_one_value;

	template <class T> T calc_gcd(T p, T q) const {
		if (p < q)
			std::swap(p, q);

		if (!q)
			return p;

		T t;
		while ((t = (p % q))) {
			p = q;	
			q = t;
		}

		return q;
	}
public:
	weighted_sum() : m_gcd(0), m_no_data_weight(0), m_fixed(true), m_has_one_value(false) {}

	const value_vector& values() const { return m_values; }
	const time_diff_vector& weights() const { return m_weights; }

	void add(const value_type& value, const time_diff_type& weight) {
		if (!m_has_one_value) {
			m_one_value = value;
			m_one_weight = weight;
			m_gcd = weight;
			m_has_one_value = true;
		} else {
			m_values.push_back(value);
			m_weights.push_back(weight);
			m_gcd = calc_gcd(weight, m_gcd);
		}

	}

	time_diff_type gcd() const {
		return calc_gcd(m_gcd, m_no_data_weight);
	}

	sum_type sum(time_diff_type& weight) const {
		if (!m_has_one_value) {
			weight = 0;
			return 0;
		}

		time_diff_type _gcd = gcd();
		weight = m_one_weight / _gcd;
		sum_type sum = m_one_value * weight;
		for (size_t i = 0; i < m_values.size(); i++) {
			time_diff_type _weight = m_weights[i] / _gcd;

			sum += _weight * m_values[i];
			weight += _weight;
		}

		return sum;
	}

	value_type avg() const {
		if (!m_has_one_value)
			return no_data<value_type>();

		time_diff_type weight;
		sum_type _sum = sum(weight);
		return _sum / weight;
	}

	time_diff_type weight() {
		if (m_weights.size()) {
			time_diff_type weight = std::accumulate(m_weights.begin(), m_weights.end(), m_one_weight);
			return weight / gcd();
		} else {
			return 0;
		}
	}
	
	time_diff_type no_data_weight() {
		if (!m_no_data_weight)
			return m_no_data_weight;
		else
			return m_no_data_weight / gcd();
	}

	void add_no_data_weight(time_diff_type _weight) {
		m_no_data_weight += _weight;
	}

	bool fixed() const { return m_fixed; }
	void set_fixed(bool fixed) { m_fixed &= fixed; }

	template<class other_value_type, class other_type>
	void take_from(weighted_sum<other_value_type, other_type>& o) {
		m_no_data_weight = o.no_data_weight();
		m_fixed = o.fixed();
		m_gcd = o.gcd();

		m_one_value = o.one_value();
		m_one_weight = o.one_weight();
		m_has_one_value = o.has_one_value();

		assert(o.weights().size() == o.values().size());
		m_values.resize(o.values().size());
		m_weights.resize(o.weights().size());
		for (size_t i = 0; i < o.values().size(); i++) {
			m_values[i] = o.values()[i];
			m_weights[i] = o.weights()[i];
		}
	}

	const value_type& one_value() const { return m_one_value; }
	const time_diff_type& one_weight() const { return m_one_weight; }
	bool has_one_value() const { return m_has_one_value; }

};

class search_condition {
public:
	virtual bool operator()(const short&) const = 0;
	virtual bool operator()(const int&) const = 0;
	virtual bool operator()(const float&) const = 0;
	virtual bool operator()(const double&) const = 0;
	virtual ~search_condition() {};
};

}

#endif
