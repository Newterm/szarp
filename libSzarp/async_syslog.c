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

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <syslog.h>
#include <stdlib.h>

#include "async_syslog.h"

static int connect_to_syslog(struct async_syslog_state *state);

static int open_log_fd(int sock_type) {
	int flags;
	int fd = socket(AF_UNIX, sock_type, 0);
	if (fd == -1)
		return -1;

	if ((flags = fcntl(fd, F_GETFL)) == -1)
		goto error;

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		goto error;

	if ((flags = fcntl(fd, F_GETFD)) == -1)
		goto error;

        if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
		goto error;
		
	return fd;
error:
	if (fd >= 0)
		close(fd);
	return -1;
}

static int connect_log_socket(int fd) {
	struct sockaddr_un logaddr;
	logaddr.sun_family = AF_LOCAL;
	strncpy(logaddr.sun_path, _PATH_LOG, sizeof(logaddr.sun_path));

again:
	if (connect(fd, (struct sockaddr *)&logaddr, sizeof(logaddr)) == -1) {
		if (errno == EAGAIN)
			goto again;
		else 
			return -1;
	}
	return 0;
}

static void write_to_syslog(struct async_syslog_state* state) {
	while (state->write_ready && !state->buffers_empty) {
		struct evbuffer* buffer = state->buffers[state->buffer_get];
		int r = evbuffer_write(buffer, state->fd);
		if (r < 0) {
			state->write_ready = 0;
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				close(state->fd);
				state->fd = -1;
				connect_to_syslog(state);
			} else {
				event_add(&state->event, NULL);
			}
		} else if (state->sock_type == SOCK_DGRAM || evbuffer_get_length(buffer) == 0) {
			if (evbuffer_get_length(buffer))
				evbuffer_drain(buffer, evbuffer_get_length(buffer));
			state->buffer_get += 1;
			state->buffer_get %= ASYNC_SYSLOG_BUFFER_SIZE;
			if (state->buffer_get == state->buffer_put)
				state->buffers_empty = 1;
		}
	}
}

static void event_callback(evutil_socket_t fd, short what, void *data) {
	struct async_syslog_state* state = data;
	state->write_ready = 1;
	write_to_syslog(state);
}

static int connect_to_syslog(struct async_syslog_state *state) {
	static const int sock_types[] = { SOCK_DGRAM, SOCK_STREAM };
	size_t i;

	if (state->fd >= 0)
		return 0;

	for (i = 0; i < sizeof(sock_types)/sizeof(sock_types[0]) && state->fd == -1; i++) {
		state->fd = open_log_fd(sock_types[i]);
		state->sock_type = sock_types[i];
		if (state->fd == -1)
			return -1;

		if (!connect_log_socket(state->fd) || errno == EINPROGRESS)
			break;

		close(state->fd);
		state->fd = -1;
	}

	if (state->fd < 0)
		return -1;

	if (event_assign(&state->event, state->base, state->fd, EV_WRITE, event_callback, state)) {
		close(state->fd);
		state->fd = -1;
		return -1;
	}

	if (errno == EINPROGRESS) {
		state->write_ready = 0;
		event_add(&state->event, NULL);
	} else {
		state->write_ready = 1;
	}

	return 0;
}

static void add_to_log_buffer(struct async_syslog_state* state, int priority, const char *format, va_list ap) {
	struct evbuffer* buffer = state->buffers[state->buffer_put];
	struct timeval tv;
	char ctime_buf[27];

	if (event_base_gettimeofday_cached(state->base, &tv))
		return;

	priority |= state->facility;
	ctime_r(&tv.tv_sec, ctime_buf);
	if (state->option & LOG_PID)
        	evbuffer_add_printf(buffer, "<%d>%.15s %s[%d]: ", priority, ctime_buf + 4, state->ident, getpid());
	else
		evbuffer_add_printf(buffer, "<%d>%.15s %s: ", priority, ctime_buf + 4, state->ident);

	evbuffer_add_vprintf(buffer, format, ap);
	if (state->sock_type == SOCK_STREAM)
		evbuffer_add_printf(buffer, "%c", 0);

	state->buffer_put += 1;
	state->buffer_put %= ASYNC_SYSLOG_BUFFER_SIZE;
	state->buffers_empty = 0;
}

struct async_syslog_state* async_openlog(struct event_base* base, const char *ident, int option, int facility) {
	size_t i = 0;
	struct async_syslog_state* state = (struct async_syslog_state*) malloc(sizeof(struct async_syslog_state));
	if (state == NULL)
		return NULL;

	state->base = base;	
	state->ident = strdup(ident);
	state->option = option;
	state->facility = facility;
	state->mask = 0xff;
	state->fd = -1;
	state->sock_type = SOCK_DGRAM;
	state->write_ready = 0;

	for (; i < ASYNC_SYSLOG_BUFFER_SIZE; i++) {
		state->buffers[i] = evbuffer_new();
		if (state->buffers[i] == NULL)
			goto error;
	}
	state->buffer_get = 0;
	state->buffer_put = 0;
	state->buffers_empty = 1;
	
	return state;
error:
	while (i)
		evbuffer_free(state->buffers[--i]);
	free(state);
	return NULL;
}

void async_syslog_setmask(struct async_syslog_state *state, int mask) {
	state->mask = mask;
}

void async_syslog(struct async_syslog_state* state, int priority, const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	async_vsyslog(state, priority, format, ap);
	va_end(ap);
}

void async_vsyslog(struct async_syslog_state* state, int priority, const char *format, va_list ap) {
	if (!(state->mask & LOG_MASK(LOG_PRI(priority))) || (priority &~ (LOG_PRIMASK|LOG_FACMASK)))
		return;

	if (state->buffer_get == state->buffer_put && !state->buffers_empty)
		return;

	connect_to_syslog(state);
	add_to_log_buffer(state, priority, format, ap);
	write_to_syslog(state);
}

void async_closelog(struct async_syslog_state *state) {
	size_t i;
	for (i = 0; i < ASYNC_SYSLOG_BUFFER_SIZE; i++)
		evbuffer_free(state->buffers[i]);
	if (state->fd >= 0) {
		event_del(&state->event);
		close(state->fd);
	}
	free(state->ident);
	free(state);
}
