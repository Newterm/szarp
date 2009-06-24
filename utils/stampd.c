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
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include <event.h>
#include <evhttp.h>

#define ADDRESS "*"
#define URI "/sync.cgi"
#define PORT 81

#define LOG_FILE PREFIX"/logs/stampd.log"


FILE * log_file = NULL;
struct evhttp* server = NULL;


void sz_log(const char * msg_format, ...) {
	va_list fmt_args;
	time_t t = time(NULL);
	struct tm *ptm = localtime(&t);
	char buf[64];

	strftime(buf, sizeof(buf), "%b %d %T ", ptm);
	fprintf(log_file, buf);

	va_start(fmt_args, msg_format);
	vfprintf(log_file, msg_format, fmt_args);
	va_end (fmt_args);

	fprintf(log_file, "\n");
	fflush(log_file);
}

/* handler for SIG_INT */
void signal_handler(int fd, short event, void *arg) {
	if (NULL != log_file) {
	    sz_log("Got signal %d, exiting", EVENT_SIGNAL((struct event *)arg));
	    fclose(log_file);
	}

	if (NULL != server)
	    evhttp_free(server);

	exit(0);
}

time_t get_mtime(char *id) {
	struct stat st;
	char *c;
	char *path;
	int ret;
	
	for (c = id; *c; c++)
		switch (*c) {
			case '0'...'9':
			case 'a'...'z':
			case 'A'...'Z':
			    break;
			default:
			    return (time_t) -1;
		}
	
	if (asprintf(&path, "/opt/szarp/%s/szbase_stamp", id) == -1)
		return (time_t) -1;
	
	ret = lstat(path, &st);
	free(path);
	
	if (0 == ret)
		return st.st_mtime;
	else
		return (time_t) -1;
}

void http_event_handler(struct evhttp_request *req, void *ptr) {

        struct evkeyvalq *decoded_uri = calloc(1, sizeof(struct evkeyvalq));
	struct evbuffer* evb;
	struct evkeyval *entry;
	const char * user_agent;
	time_t mtime = -1;

	evhttp_parse_query(req->uri, decoded_uri);

	user_agent = evhttp_find_header(req->input_headers, "User-Agent");

	TAILQ_FOREACH(entry, decoded_uri, next) {
		if (!strcmp(entry->key, "id"));
			break;
	}

	if (entry != NULL)
		mtime = get_mtime(entry->value);

	sz_log("%s %s %ld \"%s\"", req->remote_host, req->uri, (long)mtime, user_agent);

	while ((entry = TAILQ_FIRST(decoded_uri)) != NULL) {
		TAILQ_REMOVE(decoded_uri, entry, next);
		free(entry->key);
		free(entry->value);
	}
	free(decoded_uri);

	evb = evbuffer_new();
	evbuffer_add_printf(evb, "%d", (int)mtime);
	evhttp_send_reply(req, 200, "OK", evb);
}

int main(int argc, char *argv[]) {
	struct event sigint_ev;
	struct event sigterm_ev;

	log_file = fopen(LOG_FILE, "a");
	if (NULL == log_file) {
	    fprintf(stderr, "Cannot open log file <%s>, not starting\n", LOG_FILE);
	    return 1;
	}

	daemon(0, 0);

	sz_log("Starging daemon");

	event_init();

	server = evhttp_start(ADDRESS, PORT);
	if (NULL == server) {
	    sz_log("Cannot create server");
	    return 2;
	}

	evhttp_set_cb(server, URI, http_event_handler, NULL);

	event_set(&sigint_ev, SIGINT, EV_SIGNAL|EV_PERSIST, signal_handler, &sigint_ev);
	event_add(&sigint_ev, NULL);

	event_set(&sigterm_ev, SIGTERM, EV_SIGNAL|EV_PERSIST, signal_handler, &sigterm_ev);
	event_add(&sigterm_ev, NULL);

	event_dispatch();

	return 0;
}

