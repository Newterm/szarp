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
#ifndef __SZ4_UTIL_H__
#define __SZ4_UTIL_H__

class TParam;

#include "szarp_config.h"
#include "sz4/defs.h"

namespace sz4 {


template<class FT, class TT> class convert_v {
	TT m_v;
public:
	convert_v(const FT& f) {
		if (value_is_no_data(f))
			m_v = no_data<TT>();
		else
			m_v = f;
	}

	const TT& operator()() const {
		return m_v;
	}
};

template<class T> class convert_v<T, T> {
	const T& m_v;
public:
	convert_v(const T& f) : m_v(f) { }	

	const T& operator()() const {
		return m_v;
	}
};

template<class T> T scale_value(const T& v, TParam::DataType dt, int prec) {
	switch (dt) {
		case TParam::FLOAT:
		case TParam::DOUBLE:
			return v;
		case TParam::SHORT:
		case TParam::INT:
			if (value_is_no_data(v))
				return v;
			else
				return v / pow(10, prec);
		default:
			assert(false);
	}

	return SZARP_NO_DATA;
}

template<class T> T scale_value(const T& v, TParam* p) {
	return scale_value(v, p->GetDataType(), p->GetPrec());
}

template<class T> T descale_value(const T& v, TParam* p) {
	switch (p->GetDataType()) {
		case TParam::FLOAT:
		case TParam::DOUBLE:
			if (value_is_no_data(v))
				return v;
			else
				return v * pow(10, p->GetPrec());
		case TParam::SHORT:
		case TParam::INT:
			return v;
		default:
			assert(false);
	}
}

SZARP_PROBE_TYPE get_probe_type_step(SZARP_PROBE_TYPE pt);

}

#endif
