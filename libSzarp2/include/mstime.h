#ifndef __COMMON_H__
#define __COMMON_H__

#include <sys/time.h>

/** should be enough for about 69 years of ms */
typedef long long ms_time_t;

inline struct timeval ms2timeval(ms_time_t ms) {
	struct timeval tv;
	tv.tv_sec = (time_t)(ms / 1000);
	tv.tv_usec = (suseconds_t)((ms % 1000) * 1000);
	return tv;
}

#endif /* __COMMON_H__ */
