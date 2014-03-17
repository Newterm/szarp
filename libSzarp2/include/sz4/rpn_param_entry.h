#ifndef __SZ4_RPN_PARAM_ENTRY_H__
#define __SZ4_RPN_PARAM_ENTRY_H__
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

#include "szarp_base_common/definable_calculate.h"
#include "sz4/definable_param_cache.h"
#include "sz4/buffered_param_entry.h"

namespace sz4 {

class empty_fetch_functor : public value_fetch_functor {
public:
	virtual double operator()(TParam* param) {
		assert(false);
	}
	virtual ~empty_fetch_functor() {}
};


template<class types> class rpn_calculate {
	base_templ<types>* m_base;
	TParam* m_param;
public:
	rpn_calculate(base_templ<types>* base, TParam* param) : m_base(base), m_param(param) {
	}

	template<class T> std::tr1::tuple<double, bool, std::set<generic_block*> > calculate_value(T time, SZARP_PROBE_TYPE probe_type) {

		//for backward compatiblity, 'new' method of caluclating RPN definable params
		if (probe_type != PT_SEC10 || probe_type != PT_SEC || probe_type != PT_HALFSEC)
			probe_type = PT_MIN10;

		int num_of_params = m_param->GetNumParsInFormula();
		double varray[num_of_params];
		const double *varrarr[num_of_params];
		for (int i = 0; i < num_of_params; i++)
			varrarr[i] = &varray[i];

		const std::wstring& formula = m_param->GetDrawFormula();
		TParam **f_cache = m_param->GetFormulaCache();
		double pw = pow(10, m_param->GetPrec());
		double sum = 0;
		size_t count = 0;
		bool fixed = true;
		std::set<generic_block*> refferred_blocks;

		T end_time = szb_move_time(time, 1, probe_type);
		double stack[200];

		while (time < end_time) {
			T next_time = szb_move_time(time, 1, probe_type);
			for (int i = 0; i < num_of_params; i++) {
				weighted_sum<double, T> wsum;
				m_base->get_weighted_sum(f_cache[i], time, next_time, probe_type, wsum);
				varray[i] = wsum.avg() * descale_factor(f_cache[i]);
				refferred_blocks.insert(
					wsum.refferred_blocks().begin(),
					wsum.refferred_blocks().end());
				fixed &= wsum.fixed();
			}

			default_is_summer_functor is_summer(second_time_t(time), m_param);
			empty_fetch_functor empty_fetch;

			double v = szb_definable_calculate(stack,
					int(sizeof(stack) / sizeof(stack[0])),
					varrarr,
					f_cache,
					formula,
					num_of_params,
					empty_fetch,
					is_summer,
					m_param) / pw;
			if (!isnan(v)) {
				sum += v;
				count += 1;
			}

			time = next_time;
		}

		double v;
		if (count) 
			v = sum / count;
		else
			v = nan("");

		return std::tr1::tuple<double,
				bool,
				std::set<generic_block*> >(
				v,
				fixed,
				refferred_blocks);	

	}
};


template<class value_type, class time_type, class types> class rpn_param_entry_in_buffer : public buffered_param_entry_in_buffer<value_type, time_type, types, rpn_calculate> {
public:
	rpn_param_entry_in_buffer(base_templ<types>* _base, TParam* param, const boost::filesystem::wpath& path) : buffered_param_entry_in_buffer<value_type, time_type, types, rpn_calculate>(_base, param, path) {}
};



}

#endif
