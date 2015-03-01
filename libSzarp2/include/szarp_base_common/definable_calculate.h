#ifndef __SZARP_BASE_DEFINABLE_CALCULATE_H__
#define __SZARP_BASE_DEFINABLE_CALCULATE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "szarp_config.h"

class value_fetch_functor {
public:
	virtual double operator()(TParam* param) = 0;
	virtual ~value_fetch_functor() {}
};

class is_summer_functor {
public:
	virtual bool operator()(bool& is_summer) = 0;
	virtual ~is_summer_functor() {}
};

class default_is_summer_functor : public is_summer_functor {
	TParam* param;
	time_t t;
public:
	default_is_summer_functor(time_t _t, TParam *_param);
	virtual bool operator()(bool & is_summer);
	virtual ~default_is_summer_functor() {}
};

double
szb_definable_calculate(double * stack, int stack_size, const double** cache, TParam** params,
	const std::wstring& formula, int param_cnt, value_fetch_functor& value_fetch, is_summer_functor& is_summer, TParam* param);

#endif
