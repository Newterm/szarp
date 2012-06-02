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

/** Implementation of libevent based asynchronous syslog library.
 * Inspired by: http://freecode.com/projects/syslog-async
 */



#ifndef _ASYNC_SYSLOG_H__
#define _ASYNC_SYSLOG_H__

#include <syslog.h>
#include <stdarg.h>
#include <event.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ASYNC_SYSLOG_BUFFER_SIZE 20

struct async_syslog_state {
	struct event_base* base;

	struct evbuffer* buffers[ASYNC_SYSLOG_BUFFER_SIZE];
	int buffer_get;
	int buffer_put;
	int buffers_empty;

	struct event event;

	char* ident;
	int option;
	int facility;

	int mask;

	int fd;
	int sock_type;
	int write_ready;
};


struct async_syslog_state* async_openlog(struct event_base* base, const char *ident, int option, int facility);
void async_syslog_setmask(struct async_syslog_state *state, int mask);
void async_syslog(struct async_syslog_state* state, int priority, const char *fmt, ...);
void async_vsyslog(struct async_syslog_state* state, int priority, const char *format, va_list ap);
void async_closelog(struct async_syslog_state *state);

#ifdef __cplusplus
}
#endif

#endif
