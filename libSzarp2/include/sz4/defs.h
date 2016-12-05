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

#include <boost/container/flat_set.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#ifdef __arm__
	typedef double float_sum_type;
#else
	#include <boost/multiprecision/float128.hpp>
	typedef boost::multiprecision::float128 float_sum_type;
#endif


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

template<class T> T no_data(){ 
	return std::numeric_limits<T>::min();
}

template<> float no_data<float>();

template<> double no_data<double>();

template<> unsigned int no_data<unsigned int>();

template<> unsigned short no_data<unsigned short>();

template<class T> bool value_is_no_data(const T& v) {
	return v == no_data<T>();
}

bool value_is_no_data(const double& v);

bool value_is_no_data(const float& v);


template<class T> struct time_difference { };

template<> struct time_difference<second_time_t> {
	typedef long type;
};

template<> struct time_difference<nanosecond_time_t> {
	typedef boost::multiprecision::cpp_int type;
};

template<class T> struct value_sum { };

typedef boost::multiprecision::cpp_int int_sum_type;

template<> struct value_sum<short> {
	typedef int_sum_type type;
};

template<> struct value_sum<unsigned short> {
	typedef int_sum_type type;
};

template<> struct value_sum<int> {
	typedef int_sum_type type;
};

template<> struct value_sum<unsigned int> {
	typedef int_sum_type type;
};

template<> struct value_sum<float> {
	typedef float_sum_type type;
};

template<> struct value_sum<double> {
	typedef float_sum_type type;
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

template <class F, class T> struct sum_conv_helper {
	static T _do(const F& v) { return static_cast<T>(v); }
};

template<> struct sum_conv_helper<int_sum_type, short> {
	static short _do(const int_sum_type& v) { return v.convert_to<int>(); }
};

template<> struct sum_conv_helper<float_sum_type, int_sum_type> {
	static int_sum_type _do(float_sum_type v) {
		int_sum_type r(0);
		int_sum_type e(1);
		while (v >= 1) {
			if (fmod(v, 2) >= 1)
				r += e;
			e *= 2;
			v /= 2;
		}
		return r;
	}
};

template <class FV, class FT, class TV, class TT> struct wsum_converter {
};

template<class V, class T> class weighted_sum {
public:
	typedef V value_type;
	typedef T time_type;
	typedef typename value_sum<value_type>::type sum_type;
	typedef typename time_difference<time_type>::type time_diff_type;

protected:
	sum_type m_wsum;
	time_diff_type m_weight; 

	time_diff_type m_no_data_weight;
	bool m_fixed;

public:
	weighted_sum() : m_wsum(0), m_weight(0), m_no_data_weight(0), m_fixed(true) {}

	virtual void add(const value_type& value, const time_diff_type& weight) {
		m_wsum += sum_type(value) * static_cast<sum_type>(weight);
		m_weight += weight;
	}

	sum_type sum(time_diff_type& weight) const {
		weight = m_weight;
		return m_wsum;
	}

	sum_type _sum() const {
		return m_wsum;
	}

	value_type avg() const {
		if (!m_weight)
			return no_data<value_type>();

		return sum_conv_helper<sum_type, value_type>::_do(m_wsum / sum_type(m_weight));
	}

	time_diff_type weight() const {
		return m_weight;
	}
	
	time_diff_type no_data_weight() const {
		return m_no_data_weight;
	}

	void add_no_data_weight(time_diff_type _weight) {
		m_no_data_weight += _weight;
	}

	bool fixed() const { return m_fixed; }
	void set_fixed(bool fixed) { m_fixed &= fixed; }

	template<class other_value_type, class other_time_type>
	void take_from(weighted_sum<other_value_type, other_time_type>& o) {
		m_fixed = o.fixed();

		typedef weighted_sum<other_value_type, other_time_type> other_sum_type;
		typename other_sum_type::time_diff_type other_weight;
		typename other_sum_type::sum_type other_sum;

		other_sum = o.sum(other_weight);

		wsum_converter<other_value_type, other_time_type, value_type, time_type>::_do(
					other_sum, other_weight, o.no_data_weight(),
					m_wsum, m_weight, m_no_data_weight);
	}
};

template <class V1, class V2, class T> struct wsum_converter<V1, T, V2, T> {
	typedef typename weighted_sum<V1, T>::sum_type from_sum_type;
	typedef typename weighted_sum<V1, T>::time_diff_type from_time_diff_type;
	typedef typename weighted_sum<V2, T>::sum_type to_sum_type;
	typedef typename weighted_sum<V2, T>::time_diff_type to_time_diff_type;

	static void _do(const from_sum_type& _sum,
			const from_time_diff_type& _weight,
			const from_time_diff_type& _no_data_weight,
			to_sum_type& sum,
			to_time_diff_type& weight,
			to_time_diff_type& no_data_weight) {
		sum = sum_conv_helper<from_sum_type, to_sum_type>::_do(_sum);
		weight = _weight;
		no_data_weight = _no_data_weight;
	}
};

template <class V1, class V2> struct wsum_converter<V1, sz4::second_time_t, V2, sz4::nanosecond_time_t> {
	typedef typename weighted_sum<V1, second_time_t>::sum_type from_sum_type;
	typedef typename weighted_sum<V1, second_time_t>::time_diff_type from_time_diff_type;
	typedef typename weighted_sum<V2, nanosecond_time_t>::sum_type to_sum_type;
	typedef typename weighted_sum<V2, nanosecond_time_t>::time_diff_type to_time_diff_type;

	static void _do(const from_sum_type& _sum,
			const from_time_diff_type& _weight,
			const from_time_diff_type& _no_data_weight,
			to_sum_type& sum,
			to_time_diff_type& weight,
			to_time_diff_type& no_data_weight) {
		sum = sum_conv_helper<from_sum_type, to_sum_type>::_do(_sum) * 1000000000;

		weight = _weight;
		weight *= 1000000000;

		no_data_weight = _no_data_weight;
		no_data_weight *= 1000000000;	
	}
};

template <class V1, class V2> struct wsum_converter<V1, sz4::nanosecond_time_t, V2, sz4::second_time_t> {
	typedef typename weighted_sum<V1, nanosecond_time_t>::sum_type from_sum_type;
	typedef typename weighted_sum<V1, nanosecond_time_t>::time_diff_type from_time_diff_type;
	typedef typename weighted_sum<V2, second_time_t>::sum_type to_sum_type;
	typedef typename weighted_sum<V2, second_time_t>::time_diff_type to_time_diff_type;

	static void _do(const from_sum_type& _sum,
			const from_time_diff_type& _weight,
			const from_time_diff_type& _no_data_weight,
			to_sum_type& sum,
			to_time_diff_type& weight,
			to_time_diff_type& no_data_weight) {
		sum = sum_conv_helper<from_sum_type, to_sum_type>::_do(_sum / 1000000000);

		weight = static_cast<to_time_diff_type>(from_time_diff_type(_weight / 1000000000));
		no_data_weight = static_cast<to_time_diff_type>(from_time_diff_type(_no_data_weight / 1000000000));
	}
};

class search_condition {
public:
	virtual bool operator()(const short&) const = 0;
	virtual bool operator()(const int&) const = 0;
	virtual bool operator()(const float&) const = 0;
	virtual bool operator()(const double&) const = 0;
	virtual ~search_condition() {};
};

class no_data_search_condition : public search_condition {
public:
	bool operator()(const short& v) const override;
	bool operator()(const int& v) const override;
	bool operator()(const float& v) const override;
	bool operator()(const double& v) const override;
};

}

#endif
