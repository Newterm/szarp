/* 
  SZARP: SCADA software 

*/
/* Szbase defines
 * $Id$
 *
 */

#ifndef __SZARP_BASE_DEFINES__
#define __SZARP_BASE_DEFINES__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cmath>
#include <math.h>
#include <sys/types.h>

#define SZB_FILE_TYPE	int16_t	
				/**< type of probe saved in files */

#ifdef SZARP_BIG_ENDIAN
/** Conversion little->big endian for int16_t type (signed) */
static inline SZB_FILE_TYPE szbfile_endian(SZB_FILE_TYPE n)
{
	SZB_FILE_TYPE ret;
	unsigned char *p = (unsigned char *)&n;
	unsigned char *rp = (unsigned char *)&ret;
	rp[0] = p[1];
	rp[1] = p[0];
	return ret;
	
}
#else
#define szbfile_endian(n)	(n)
#endif

#define SZB_FILE_NODATA	szbfile_endian(SHRT_MIN)
				/**< no data value saved in file */

#define SZBASE_TYPE 	double	/**< type of probe in cache block */
#define SZB_NODATA 	nan("")	/**< nodata value for applications */
#define IS_SZB_NODATA(v) \
	std::isnan(v)		/**< checks if value is 'no data' */

#define SZBASE_MAX_DATA_IN_BLOCK 4608 // TODO sprawdzic
#define SZBASE_PROBES_IN_BLOCK 1800

#define SZBASE_CMASK 0664	/**< Mode for creat() used when creating new files. */
#define SZBASE_CMASK_DIR 0775	/**< Mode for mkdir() used when creating directories. */

/**
 * Format string for creating SzarpBase file names from parameter
 * name, year and month.
 */
#define SZBASE_FILENAME_FORMAT	L"%s/%04d%02d.szb"
/* Extra space in file name (except for param name), default
 * value. */


#define SZBASE_MIN_YEAR	0	/**< Minimal year for use in base. */
#define SZBASE_MAX_YEAR	9999	/**< Maximum year for use in base. */
#define SZBASE_DATA_SPAN	600	/**< Length in seconds of data saved to base. */
#define SZBASE_PROBE_SPAN	10	/**< Length in seconds of probe stroed in base. */
#define SZBASE_DATA_PER_DAY (24*(3600/600))	/**< Number of probes per day. */


/* SzarpBase error codes. */
#define SZBE_OK		0	/**< no error */
#define SZBE_SYSERR	-1	/**< system error, check errno variable */
#define SZBE_INVDATE	-2	/**< invalid year or month */
#define SZBE_INVPARAM	-3	/**< illegal parameter name */
#define SZBE_NULL	-4	/**< Null parameter for function where it's not allowed */
#define SZBE_OUTMEM	-5	/**< out of memory */
#define SZBE_LUA_ERROR  -6	/**< error in lua param execution/compilation */
#define SZBE_CONN_ERROR -7	/**< error in connection with probe server */
#define SZBE_SEARCH_TIMEOUT -8	/**< error in connection with probe server */

typedef void (*szb_custom_function) (struct tm* time, SZBASE_TYPE* stack, short* sp);

void szb_register_custom_function(wchar_t symbol, szb_custom_function function);

enum SZB_BLOCK_TYPE {
	MIN10_BLOCK = 0,
	SEC10_BLOCK,
	UNUSED_BLOCK_TYPE,
};

#endif // __SZARP_BASE_DEFINES__
