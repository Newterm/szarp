#ifndef __SZBASE_DEFINABLE_CALCULATE_H__
#define __SZBASE_DEFINABLE_CALCULATE_H__

#include "szarp_base_common/definable_calculate.h"
#include "szbase/szbbuf.h"

class szbase_value_fetch_functor : public value_fetch_functor {
	szb_buffer_t *m_buffer;
	time_t m_time;
	SZARP_PROBE_TYPE m_pt;
public:
	szbase_value_fetch_functor(szb_buffer_t *buffer, time_t time, SZARP_PROBE_TYPE pt);
	virtual double operator()(TParam* param);
	~szbase_value_fetch_functor() {}
};

#endif
