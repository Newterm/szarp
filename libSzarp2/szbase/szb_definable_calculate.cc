#include "szb_definable_calculate.h"

szbase_value_fetch_functor::szbase_value_fetch_functor(szb_buffer_t *buffer, time_t time, SZARP_PROBE_TYPE pt) : m_buffer(buffer), m_time(time), m_pt(pt) { }

double szbase_value_fetch_functor::operator()(TParam* param) {
	return szb_get_data(m_buffer, param, m_pt);
}

