#ifndef __BLOCK_H__
#define __BLOCK_H__
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

#include <algorithm>

namespace sz4 {

template<class V, class T> struct value_time_pair {
	typedef V value_type;
	typedef T time_type;

	value_type value;
	time_type  time;
} __attribute__ ((packed));

template<class V, class T> struct value_time_pair<V, T> make_value_time_pair(V v, T t) {
	value_time_pair<V, T> p;
	p.value = v;
	p.time = t;
	return p;
}

template<class V, class T> bool cmp_time(const value_time_pair<V, T>& p, const T& t) {
	return p.time < t;
}

template<class I, class T> I search_entry_for_time(I begin, I end, T t) {
	typedef typename I::value_type pair_type;
	typedef typename pair_type::value_type value_type;

	return std::lower_bound(begin, end, t, cmp_time<value_type, T>);
}

template<class V, class T> class block {
public:
	typedef V data_type;
	typedef T time_type;

	typedef value_time_pair<V, T> value_time_type;

	typedef std::vector<value_time_type> value_time_vector_pair;

	data_type m_start_time;
	time_type m_end_time;

	value_time_vector_pair m_data;

};

}

#endif
