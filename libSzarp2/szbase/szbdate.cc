/* 
  SZARP: SCADA software 

*/
/*
 * szbase
 * $Id$
 * <pawel@praterm.com.pl>
 */

#include "szbdate.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

int szb_daysinmonth(int year, int month)
{
    static int dm[14] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0 };
    
    if (month == 2) {
	if ((year % 400) == 0)
	    return 29;
	if ((year % 100) == 0)
	    return 28;
	if ((year % 4) == 0)
	    return 29;
    }
    return dm[month];
}

int szb_probecnt(const int year, const int month)
{
    static int probes[14] = { 0,
		    31 * SZBASE_DATA_PER_DAY,
		    28 * SZBASE_DATA_PER_DAY,
		    31 * SZBASE_DATA_PER_DAY,
		    30 * SZBASE_DATA_PER_DAY,
		    31 * SZBASE_DATA_PER_DAY,
		    30 * SZBASE_DATA_PER_DAY,
		    31 * SZBASE_DATA_PER_DAY,
		    31 * SZBASE_DATA_PER_DAY,
		    30 * SZBASE_DATA_PER_DAY,
		    31 * SZBASE_DATA_PER_DAY,
		    30 * SZBASE_DATA_PER_DAY,
		    31 * SZBASE_DATA_PER_DAY,
		    0 };
    
    if ((year < SZBASE_MIN_YEAR) || (year > SZBASE_MAX_YEAR))
	return 0;

    if (month == 2) {
	if ((year % 400) == 0)
	    return 29 * SZBASE_DATA_PER_DAY;
	if ((year % 100) == 0)
	    return 28 * SZBASE_DATA_PER_DAY;
	if ((year % 4) == 0)
	    return 29 * SZBASE_DATA_PER_DAY;
    }

    return probes[month];
}

int szb_time2my(time_t time, int * year, int * month)
{
	struct tm * ptmp;
#ifdef HAVE_GMTIME_R
	struct tm tmp;
#endif
	
	if (year == NULL)
		return 2;
	if (month == NULL)
		return 3;
	
#ifdef HAVE_GMTIME_R
	ptmp = gmtime_r(&time, &tmp);
#else
	ptmp = gmtime(&time);
#endif
	if (ptmp == NULL)
		return 1;
	*year = ptmp->tm_year + 1900;
	*month = ptmp->tm_mon + 1;
	return 0;
}

int day_sec[32] = {
    0, 0, 86400, 2 * 86400, 3 * 86400, 4 * 86400, 5 * 86400, 6 * 86400,
    7 * 86400, 8 * 86400, 9 * 86400, 10 * 86400, 11 * 86400, 12 * 86400, 13 * 86400, 14 * 86400, 
    15 * 86400, 16 * 86400, 17 * 86400, 18 * 86400, 19 * 86400, 20 * 86400, 21 * 86400, 22 * 86400,
    23 * 86400, 24 * 86400, 25 * 86400, 26 * 86400, 27 * 86400, 28 * 86400, 29 * 86400, 30 * 86400,
};

int hour_sec[24] = {
    0,   3600,  2 * 3600, 3 * 3600, 4 * 3600, 5 * 3600, 6 * 3600, 7 * 3600,
    8 * 3600, 9 * 3600, 10 * 3600, 11 * 3600, 12 * 3600, 13 * 3600, 14 * 3600, 15 * 3600, 
    16 * 3600, 17 * 3600, 18 * 3600, 19 * 3600, 20 * 3600, 21 * 3600, 22 * 3600, 23 * 3600
};

int szb_probeind(time_t t, time_t probe_length)
{
#ifndef HAVE_GMTIME_R
    struct tm * ptm;
#endif
    struct tm tmp;
//    time_t ret;
    
#ifdef HAVE_GMTIME_R
    gmtime_r(&t, &tmp);
#else
    ptm = gmtime(&t);
    memcpy(&tmp, ptm, sizeof(struct tm));
#endif

    return (day_sec[tmp.tm_mday] + hour_sec[tmp.tm_hour] + tmp.tm_min * 60 + tmp.tm_sec) / probe_length;
}

time_t probe2time(int probe, int year, int month)
{
	struct tm tm;
	time_t t;
	
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = 1;
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;

	t = timegm(&tm);
	
	return t + (probe * SZBASE_DATA_SPAN);
}

void probe2gmt(int probe, int year, int month, struct tm * tm)
{
	time_t t;
#ifndef HAVE_GMTIME_R
	struct tm *ptm;
#endif
	
	t = probe2time(probe, year, month);
#ifdef HAVE_GMTIME_R
	gmtime_r(&t, tm);
#else
	ptm = gmtime(&t);
	memcpy(tm, ptm, sizeof(struct tm));
#endif
	
}

void probe2local(int probe, int year, int month, struct tm * tm)
{
	time_t t;
#ifndef HAVE_LOCALTIME_R
	struct tm * ptm;
#endif
	t = probe2time(probe, year, month);
#ifndef HAVE_LOCALTIME_R
	ptm = localtime(&t);
	memcpy(tm, ptm, sizeof(struct tm));
#else
	localtime_r(&t, tm);
#endif
}

time_t szb_round_time(time_t t, SZARP_PROBE_TYPE probe_type, int custom_length)
{
	struct tm tm;
#ifndef HAVE_LOCALTIME_R
	struct tm *ptm;
#endif
	
	if (t <= 0)
		return -1;
	if ( (probe_type == PT_CUSTOM) && (custom_length <= 0) )
		return -1;
	
	switch (probe_type) {
		case PT_CUSTOM :
			return (t - (t % custom_length));
		case PT_MIN10 :
			return (t - (t % 600));
		case PT_SEC10:
			return (t - (t % 10));
		case PT_HOUR :
			return (t - (t % 3600));
		case PT_HOUR8 :
#ifndef HAVE_LOCALTIME_R
			ptm = localtime(&t);
			memcpy(&tm, ptm, sizeof(struct tm));
#else
			localtime_r(&t, &tm);
#endif
			tm.tm_sec = tm.tm_min = 0;
			tm.tm_hour -= tm.tm_hour % 8;
			tm.tm_isdst = -1;
			return mktime(&tm);
		case PT_DAY :
#ifndef HAVE_LOCALTIME_R
			ptm = localtime(&t);
			memcpy(&tm, ptm, sizeof(struct tm));
#else
			localtime_r(&t, &tm);
#endif
			tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
			tm.tm_isdst = -1;
			return mktime(&tm);
		case PT_WEEK :
#ifndef HAVE_LOCALTIME_R
			ptm = localtime(&t);
			memcpy(&tm, ptm, sizeof(struct tm));
#else
			localtime_r(&t, &tm);
#endif
			tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
			tm.tm_mday += tm.tm_wday - 1;
			tm.tm_isdst = -1;
			return mktime(&tm);
		case PT_MONTH :
#ifndef HAVE_LOCALTIME_R
			ptm = localtime(&t);
			memcpy(&tm, ptm, sizeof(struct tm));
#else
			localtime_r(&t, &tm);
#endif
			tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
			tm.tm_mday = 1;
			tm.tm_isdst = -1;
			return mktime(&tm);
	}
	return -1;
}

time_t szb_move_time(time_t t, int count, SZARP_PROBE_TYPE probe_type,
		                int custom_length)
{
	struct tm tm;
#ifndef HAVE_LOCALTIME_R
	struct tm *ptm;
#endif
	if (t <= 0)
		return -1;
	if ( (probe_type == PT_CUSTOM) && (custom_length <= 0) )
		return -1;
	
	switch (probe_type) {
		case PT_CUSTOM :
			return (t + custom_length * count);
		case PT_MIN10 :
			return (t + (count * 600 ));
		case PT_SEC10 :
			return (t + (count * 10 ));
		case PT_HOUR :
			return (t + (count * 3600));
		case PT_HOUR8 :
#ifndef HAVE_LOCALTIME_R
			ptm = localtime(&t);
			memcpy(&tm, ptm, sizeof(struct tm));
#else
			localtime_r(&t, &tm);
#endif
			tm.tm_hour += count * 8;
			tm.tm_isdst = -1;
			return mktime(&tm);
		case PT_DAY :
#ifndef HAVE_LOCALTIME_R
			ptm = localtime(&t);
			memcpy(&tm, ptm, sizeof(struct tm));
#else
			localtime_r(&t, &tm);
#endif
			tm.tm_mday += count;
			tm.tm_isdst = -1;
			return mktime(&tm);
		case PT_WEEK :
#ifndef HAVE_LOCALTIME_R
			ptm = localtime(&t);
			memcpy(&tm, ptm, sizeof(struct tm));
#else
			localtime_r(&t, &tm);
#endif
			tm.tm_mday += count * 7;
			tm.tm_isdst = -1;
			return mktime(&tm);
		case PT_MONTH :
#ifndef HAVE_LOCALTIME_R
			ptm = localtime(&t);
			memcpy(&tm, ptm, sizeof(struct tm));
#else
			localtime_r(&t, &tm);
#endif
			tm.tm_mon += count;
			tm.tm_isdst = -1;
			return mktime(&tm);
	}
	return -1;
}

time_t
szb_round_to_probe_block_start(time_t t) {
	return t - (t % (SZBASE_PROBES_IN_BLOCK * SZBASE_PROBE_SPAN));
}
