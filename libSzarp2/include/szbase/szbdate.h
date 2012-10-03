/* 
  SZARP: SCADA software 

*/
/*
 * szbase - szbdate.h
 * $Id$
 * <pawel@praterm.com.pl>
 */

#ifndef __SZBDATE_H__
#define __SZBDATE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include "szbdefines.h"
#include "szarp_base_common/defines.h"
#include "szarp_base_common/time.h"

		
/**
 * Return number of days in month of given year.
 * @param year year 
 * @param month month (from 1 to 12)
 * @return number of days in month, 0 on error (incorrect month)
 */
int
szb_daysinmonth(const int year, const int month);

/**
 * Return number of probes for given year and month.
 * @param year year between 0 and 9999
 * @param month month (between 1 and 12)
 * @return number of probes for full month, 0 on error (incorrect month)
 */
int
szb_probecnt(const int year, const int month);

/**
 * Converts time value to year and month.
 * @param time time to convert (EPOC value)
 * @param year pointer to year value, it is 'out' argument, filled by function,
 * if NULL function fails.
 * @param month pointer to month value, out argument, error if NULL, month
 * indexes are from 1 to 12
 * @return 0 on success or index of incorrect param (starting from 1)
 */
int
szb_time2my(time_t time, int * year, int * month);

/**
 * Return index of given time in file.
 * @param t time of probe
 * @param probe_length length of probe in file in seconds
 * @return -1 on error or non-negative index
 */
int
szb_probeind(time_t t, time_t probe_length = SZBASE_DATA_SPAN);

/**
 * Converts month, year and probe index to time value.
 * @param probe probe index (from 0)
 * @param year year (full 4 digits)
 * @param month month (from 1 to 12)
 * @return corresponding time value, -1 on error (incorrect year)
 */
time_t
probe2time(int probe, int year, int month);

/**
 * Converts month, year and probe index to broken time representation
 * in GMT.
 * @param probe probe index (from 0)
 * @param year year (full 4 digits)
 * @param month month (from 1 to 12)
 * @param tm return corresponding time structure
 */
void
probe2gmt(int probe, int year, int month, struct tm * tm);

/**
 * Converts month, year and probe index to broken time representation
 * in local time.
 * @param probe probe index (from 0)
 * @param year year (full 4 digits)
 * @param month month (from 1 to 12)
 * @param tm return corresponding time structure
 */
void
probe2local(int probe, int year, int month, struct tm * tm);

/**
 * Rounds time value according to given probe type. Uses local time.
 * @param t time value
 * @param probe_type type of probe
 * @param custom_length length of probe for PT_CUSTOM probe type (greater then
 * 0), ignored for other probe types
 * @return rounded value
 */
time_t
szb_round_time(time_t t, SZARP_PROBE_TYPE probe_type, int custom_length);

time_t
szb_round_to_probe_block_start(time_t t);

#endif

