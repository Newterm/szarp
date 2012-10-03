#ifndef __SZARP_BASE_COMMON_TIME_H__
#define __SZARP_BASE_COMMON_TIME_H__

#include "config.h"

#include <time.h>

#include "szarp_base_common/defines.h"
#include "szarp_base_common/time.h"

/** Moves time value by specified number of probes. Count value can be
 * negative to indicate left direction of move. For example
 * szb_move_time(t, -2, PT_WEEK, 0) moves t value 2 weeks back
 * @param t time value
 * @param count number of probes to move
 * @param probe_type type of probe
 * @param custom_length length of probe for probe_type PT_CUSTOM, ignored for
 * other probe types
 * @return modified time
 */
time_t
szb_move_time(time_t t, int count, SZARP_PROBE_TYPE probe_type, 
		int custom_length);


#endif

