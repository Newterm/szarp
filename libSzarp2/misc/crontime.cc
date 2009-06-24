/* 
  SZARP: SCADA software 

*/
/*
 * SZARP
 * 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#include <limits>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "liblog.h"
#include "crontime.h"


/** States of automat for parsing crontab time */
typedef enum {PTAS_START, PTAS_DIGIT, PTAS_COMMA,
		PTAS_PERIOD, PTAS_ALL}  PT_AUTO_STATES;

int parse_cron_time(const char *string, ExecTimeType *mask, int max, int min, 
		int minutes)
{
	assert(max < 32);
	assert(min >= 0);
	assert (min < max);
	assert(string != NULL);
	assert(mask != NULL);
	PT_AUTO_STATES state = PTAS_START;

	ExecTimeType bits = 0;

	int n = 0, m = 0, i;

	const char *c;

	for (c = string; ; c++) {
		switch (state) {
			case PTAS_START:
				if (*c == 0)
					goto end;
				if (*c == '*') {
					/* mark all */
					for (i = min; i <= max; i++)
						bits |= (1 << (i - min));
					state = PTAS_ALL;
				} else if (isdigit(*c)) {
					n = *c - '0';
					state = PTAS_DIGIT;
				} else {
					sz_log(1, "error: unexpected character '%c'", *c);
					goto error;
				}
				break;
			case PTAS_ALL:
				if (*c == 0)
					goto end;
				sz_log(1,"error: extra character after '*'");
				goto error;
			case PTAS_DIGIT:
				if ((*c == ',') || (*c == '-') || (*c == 0)) {
					if (minutes)
						n = (n / 10);
					if (n < min) {
						sz_log(1,"error: at least '%d' expected, got '%d'",
								min, n);
						goto error;
					}
					if (n > max) {
						sz_log(1,"error: no more than '%d' expected, got '%d'",
								max, n);
						goto error;
					}
				}
				if ((*c == ',') || (*c == 0)) {
					bits |= (1 << (n - min));
					state = PTAS_COMMA;
				} else if (*c == '-') {
					state = PTAS_PERIOD;
					m = -1;
				} else if (isdigit(*c)) {
					n = n * 10 + (*c - '0');
				} else {
					sz_log(1,"error: unexpected character '%c', expected ',' '-' or digit", *c);
					goto error;
				}
				if (*c == 0)
					goto end;
				break;
			case PTAS_COMMA:
				if (isdigit(*c)) {
					n = *c - '0';
					state = PTAS_DIGIT;
				} else {
					sz_log(1,"error: unexpected character '%c', digit expected after ','", *c);
					goto error;
				}
				break;
			case PTAS_PERIOD:
				if (isdigit(*c)) {
					if (m < 0)
						m = *c - '0';
					else 
						m = m * 10 + (*c - '0');
				} else  if ((*c == ',') || (*c == 0)) {
					if (minutes)
						m = m / 10;
					if (m < min) {
						sz_log(1,"error: at least '%d' expected, got '%d'",
								min, m);
						goto error;
					}
					if (m > max) {
						sz_log(1,"error: no more than '%d' expected, got '%d'",
								max, m);
						goto error;
					}
					if (n > m) {
						sz_log(1,"error: start of period grater then end (%d > %d)",
								n, m);
						goto error;
					}
					for (i = n - min; i < m - min; i++) {
						bits |= (1 << i);
					}
				} else {
					sz_log(1, "error: unexpected character '%c'", *c);
					goto error;
				}
				if (*c == 0)
					goto end;
				break;
		}
	}
end:
	*mask = bits;
	return 0;
error:
	return 1;
		
}
