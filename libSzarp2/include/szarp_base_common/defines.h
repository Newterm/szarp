#ifndef __SZARP_BASE_DEFINES_H__
#define __SZARP_BASE_DEFINES_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/** Enumeration type for different probe types. */
typedef enum { PT_FIRST = 0, PT_MSEC10 = PT_FIRST, PT_HALFSEC, PT_SEC, PT_SEC10, PT_MIN10, PT_HOUR, PT_HOUR8, PT_DAY, PT_WEEK, PT_MONTH, PT_YEAR, PT_CUSTOM, PT_LAST = PT_CUSTOM } SZARP_PROBE_TYPE ;


#endif
