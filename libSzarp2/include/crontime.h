/* 
  SZARP: SCADA software 

*/
/*
 * SZARP
 * 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */


#include <stdint.h>

/** Type for section execute time bitmask. */
#define ExecTimeType    uint32_t


/** Parses time in crontab format. 
 * @param string time description
 * @param mask return argument - masks bits corresponding to values in string
 * @param max maximum index in mask
 * @param min minimum index in mask
 * @param minutes is this is a 'minutes' string, if different than 0 then values are divided by 10
 * @return 1 on error (mask is not changed), 0 on success
 * Example: parse_cron_time("1,4-7", &mask, 8) should set mask to
 * 01111001.
 */
int parse_cron_time(const char *string, ExecTimeType *mask, int max, int min = 1, int minutes = 0);
