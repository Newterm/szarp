#ifndef __SZ4_COMBINED_PARAM_ENTRY_H__
#define __SZ4_COMBINED_PARAM_ENTRY_H__
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

#include "sz4/definable_param_cache.h"
#include "sz4/buffered_param_entry.h"
#include "sz4/util.h"

namespace sz4 {

template<class T> class unsigned_short_weighted_sum : public weighted_sum<short, T> {
public:
	typedef weighted_sum<short, T> parent_type;
	void add(const typename parent_type::value_type& value, const typename parent_type::time_diff_type& weight) {
		this->m_wsum += (unsigned short)(value) * static_cast<typename parent_type::sum_type>(weight);
		this->m_weight += weight;
	}

};

template<class types> class combined_calculate {
	typename types::base* m_base;
	TParam* m_param;

public:
	typedef typename types::base base;

	combined_calculate(typename types::base* base, TParam* param) : m_base(base), m_param(param) {
	}

	template<class T> std::tr1::tuple<double, bool> calculate_value(T time, SZARP_PROBE_TYPE probe_type) {
		unsigned_short_weighted_sum<T> ws_lsw;
		weighted_sum<short, T> ws_msw;

		typedef typename weighted_sum<short, T>::sum_type sum_type;
		sum_type sum_msw, sum_lsw;

		typename weighted_sum<unsigned short, T>::time_diff_type weight_msw, weight_lsw;

		TParam **f_cache = m_param->GetFormulaCache();
		m_base->get_weighted_sum(f_cache[0], time,
					T(szb_move_time(time, 1, probe_type)),
					probe_type, ws_msw);
		m_base->get_weighted_sum(f_cache[1], time,
					T(szb_move_time(time, 1, probe_type)),
					probe_type, (weighted_sum<short, T>&) ws_lsw);

		double v;

		sum_msw = ws_msw.sum(weight_msw);
		sum_lsw = ws_lsw.sum(weight_lsw);

		if (weight_msw) {
			if (weight_msw > weight_lsw)
				sum_lsw += sum_type(0x8000) * (weight_msw - weight_lsw);

			sum_type sum = sum_msw * 65536 + sum_lsw;

			int _iv = static_cast<int>(sum_type(sum / weight_msw));

			v = _iv;
		} else
			v = no_data<double>();


		return std::tr1::tuple<double, bool>(v, ws_msw.fixed() && ws_lsw.fixed());
	}

};

template<class value_type, class time_type, class types> class combined_param_entry_in_buffer : public buffered_param_entry_in_buffer<value_type, time_type, types, combined_calculate> {

public:
	combined_param_entry_in_buffer(typename types::base* _base, TParam* param, const boost::filesystem::wpath& path) : buffered_param_entry_in_buffer<value_type, time_type, types, combined_calculate>(_base, param, path) {}
};


}

#endif
