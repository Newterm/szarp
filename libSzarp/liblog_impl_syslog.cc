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

#include "config.h"

#ifndef MINGW32

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <string.h>

#include "liblog.h"

char * __logname = NULL;

void* sz_log_syslog_init(int level, const char* logname, SZ_LIBLOG_FACILITY facility, void *context) {
	if (NULL != logname) {
		const char *c = strrchr(logname, '/');
		if (NULL == c) {
			__logname = strdup(logname);
		}
		else {
			__logname = strdup(c + 1);
		}
	}

	openlog(__logname, LOG_PID | LOG_NDELAY, facility);
	int mask = LOG_UPTO(sz_level_to_syslog_level(level));
	setlogmask(mask);
	return NULL;
}

void sz_log_syslog_close(void* data) {
	if (NULL != __logname)
		free(__logname);
	closelog();
}

void sz_log_syslog_vlog(void* data, int level, const char * msg_format, va_list fmt_args) {
	vsyslog(sz_level_to_syslog_level(level), msg_format, fmt_args);
}

#endif
