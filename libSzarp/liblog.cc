/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#include "liblog.h"

#include "liblog_impl_syslog.h"
#include "liblog_impl_classic.h"

//init defaults
sz_log_init_function* __log_init_func = sz_log_classic_init;
sz_vlog_function* __log_vlog_func = sz_log_classic_vlog;
sz_log_close_function* __log_close_func = sz_log_classic_close;

void* __state;

int sz_loginit_cmdline(int level, const char * logfile, int *argc, char *argv[], SZ_LIBLOG_FACILITY facility)
{
	int i;
	int l;
	char *endptr, *endptr1;

	for (i = 1; i < *argc; i++) {
		if (!strncmp(argv[i], "--debug=", 8)) {
			l = strtol(argv[i]+8, &endptr, 10);
			if ((argv[i][8] == 0) || ((*endptr) != 0))
				goto fail;
			(*argc)--;
			if (i != *argc) {
				endptr = argv[i];
				memmove(argv+i, argv+i+1, (*argc-i) * 
						(sizeof(char *)));
				argv[*argc] = endptr;
			}
			return sz_loginit(l, logfile, facility);
		}
		if (!strcmp(argv[i], "-d")) {
			if (*argc <= i+1)
				goto fail;	
			l = strtol(argv[i+1], &endptr, 10);
			if (*endptr != 0)
				goto fail;

			(*argc) -= 2;
			if (i != *argc) {
				endptr = argv[i];
				endptr1 = argv[i+1];
				memmove(argv+i, argv+i+2, (*argc-i) * 
						(sizeof(char *)));
				argv[*argc-1] = endptr;
				argv[*argc] = endptr1;
			}
			return sz_loginit(l, logfile, facility);
		}
	}
fail:	
	return sz_loginit(level, logfile, facility);
}

void sz_log_system_init(sz_log_init_function init, sz_vlog_function vlog, sz_log_close_function close) {
	__log_init_func = init;
	__log_vlog_func = vlog;
	__log_close_func = close;
}

int sz_loginit(int level, const char * logname, SZ_LIBLOG_FACILITY facility, void *context) {
	// asyslog setup handled diferently
	if (NULL != __state) {
		__state = __log_init_func(level, logname, facility, context);
		return level;
	}

	// cleanup before appling new setup
	sz_logdone();

	// if no logfile setup logging to classic system with stderr output
	if (NULL == logname) {
		__log_init_func = sz_log_classic_init;
		__log_vlog_func = sz_log_classic_vlog;
		__log_close_func = sz_log_classic_close;
	}
	else {
#ifndef MINGW32
		__log_init_func = sz_log_syslog_init;
		__log_vlog_func = sz_log_syslog_vlog;
		__log_close_func = sz_log_syslog_close;
#else
		__log_init_func = sz_log_classic_init;
		__log_vlog_func = sz_log_classic_vlog;
		__log_close_func = sz_log_classic_close;
#endif
	}
	__state = __log_init_func(level, logname, facility, context);
	return level;
}

void sz_logdone(void) {
	__log_close_func(__state);
}

void sz_log(int level, const char * msg_format, ...) {
	va_list fmt_args;
	va_start(fmt_args, msg_format);
	__log_vlog_func(__state, level, msg_format, fmt_args);
	va_end(fmt_args);
}

void vsz_log(int level, const char * msg_format, va_list fmt_args) {
	__log_vlog_func(__state, level, msg_format, fmt_args);
}

int sz_level_to_syslog_level(int level) {
	switch (level) {
		case 0:
			return LOG_CRIT;
		case 1:
			return LOG_ERR;
		case 2:
			return LOG_WARNING;
		case 3:
			return LOG_NOTICE;
		case 4:
			return LOG_INFO;
		default:
			return LOG_DEBUG;
	}
}
