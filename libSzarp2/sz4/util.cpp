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

#include "sz4/util.h"
#include "custom_assert.h"
#include "protobuf/paramsvalues.pb.h"

#include <math.h>

namespace sz4 {

SZARP_PROBE_TYPE get_probe_type_step(SZARP_PROBE_TYPE pt) {
	switch (pt) {
		case PT_MSEC10:
		case PT_HALFSEC:
		case PT_SEC10:
		case PT_SEC:
		case PT_MIN10:
			return pt;
		default:
			return PT_MIN10;
	}
}

template <typename RV, typename IV>
RV cast_with_precision(IV val, int32_t prec_adj) = delete;

template <>
int64_t cast_with_precision(double val, int32_t prec_adj) { return rint(val * prec_adj); }

template <>
uint64_t cast_with_precision(double val, int32_t prec_adj) { return rint(val * prec_adj); }

template <>
double cast_with_precision(double val, int32_t prec_adj) { return val; }

template <>
int64_t cast_with_precision(int64_t val, int32_t prec_adj) { return val; }

template <>
uint64_t cast_with_precision(int64_t val, int32_t prec_adj) { return (uint64_t) val; }

template <>
double cast_with_precision(int64_t val, int32_t prec_adj) { return ((double) val) / prec_adj; }

template <>
double cast_with_precision(uint64_t val, int32_t prec_adj) { return ((double) val) / prec_adj; }

template <typename VT>
value_time_pair<VT, nanosecond_time_t> cast_param_value(const szarp::ParamValue& pv, int32_t prec_adj) {
	value_time_pair<VT, nanosecond_time_t> ret;

	if (!pv.IsInitialized() || pv.is_nan()) {
		ret.value = sz4::no_data<VT>();
		ret.time = sz4::time_trait<nanosecond_time_t>::invalid_value;
		return ret;
	}

	ret.time.second = pv.time();
	if (pv.has_nanotime()) {
		ret.time.nanosecond = pv.nanotime();
	} else {
		ret.time.nanosecond = 0;
	}

	if (pv.has_int_value()) {
		ret.value = cast_with_precision<VT>((int64_t) pv.int_value(), prec_adj);
	} else if (pv.has_double_value()) {
		ret.value = cast_with_precision<VT>((double) pv.double_value(), prec_adj);
	} else {
		ASSERT(!"Unknown or no type in ParamValue");
		ret.value = sz4::no_data<VT>();
	}

	return ret;
}

template <>
value_time_pair<int16_t, nanosecond_time_t> cast_param_value<int16_t>(const szarp::ParamValue& pv, int32_t prec_adj) {
	value_time_pair<int16_t, nanosecond_time_t> ret;
	auto i64_p = cast_param_value<int64_t>(pv, prec_adj);
	ret.value = i64_p.value;
	ret.time = i64_p.time;
	return ret;
}

template <>
value_time_pair<uint16_t, nanosecond_time_t> cast_param_value<uint16_t>(const szarp::ParamValue& pv, int32_t prec_adj) {
	value_time_pair<uint16_t, nanosecond_time_t> ret;
	auto ui64_p = cast_param_value<uint64_t>(pv, prec_adj);
	ret.value = ui64_p.value;
	ret.time = ui64_p.time;
	return ret;
}

template <>
value_time_pair<int32_t, nanosecond_time_t> cast_param_value<int32_t>(const szarp::ParamValue& pv, int32_t prec_adj) {
	value_time_pair<int32_t, nanosecond_time_t> ret;
	auto i64_p = cast_param_value<int64_t>(pv, prec_adj);
	ret.value = i64_p.value;
	ret.time = i64_p.time;
	return ret;
}

template <>
value_time_pair<uint32_t, nanosecond_time_t> cast_param_value<uint32_t>(const szarp::ParamValue& pv, int32_t prec_adj) {
	value_time_pair<uint32_t, nanosecond_time_t> ret;
	auto ui64_p = cast_param_value<uint64_t>(pv, prec_adj);
	ret.value = ui64_p.value;
	ret.time = ui64_p.time;
	return ret;
}

template <>
value_time_pair<float, nanosecond_time_t> cast_param_value<float>(const szarp::ParamValue& pv, int32_t prec_adj) {
	value_time_pair<float, nanosecond_time_t> ret;
	auto d64_p = cast_param_value<double>(pv, prec_adj);
	ret.value = d64_p.value;
	ret.time = d64_p.time;
	return ret;
}

template value_time_pair<int64_t, nanosecond_time_t> cast_param_value(const szarp::ParamValue& pv, int32_t prec_adj);
template value_time_pair<uint64_t, nanosecond_time_t> cast_param_value(const szarp::ParamValue& pv, int32_t prec_adj);

template value_time_pair<double, nanosecond_time_t> cast_param_value(const szarp::ParamValue& pv, int32_t prec_adj);

void pv_set_time(szarp::ParamValue& pv, const sz4::second_time_t& t) {
	pv.set_time(t);
}

void pv_set_time(szarp::ParamValue& pv, const sz4::nanosecond_time_t& t) {
	pv.set_time(t.second);
	pv.set_nanotime(t.nanosecond);
}


} // namespace util
