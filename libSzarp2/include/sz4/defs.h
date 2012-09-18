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
#include <stdint.h>

#include "sz4/time.h"

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

template<class T> struct time_difference { };

template<> struct time_difference<second_time_t> {
	typedef long type;
};

template<> struct time_difference<nanosecond_time_t> {
	typedef long long type;
};

template<class T> struct value_sum { };

template<> struct value_sum<short> {
	typedef int type;
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

template<class value_type, class time_type> struct weighted_sum {
	typedef typename value_sum<value_type>::type sum_type;
	typedef typename time_difference<time_type>::type time_diff_type;

	sum_type m_sum;
	time_diff_type m_weight;
	time_diff_type m_no_data_weight;

	weighted_sum() : m_sum(0), m_weight(0), m_no_data_weight(0) {}
	sum_type& sum() { return m_sum; }
	time_diff_type& weight() { return m_weight; }
	time_diff_type& no_data_weight() { return m_no_data_weight; }
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
