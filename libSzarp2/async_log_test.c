#include <stdlib.h>
#include "async_syslog.h"

struct event timer;
struct event_base* base;
struct async_syslog_state* log_state;

void cycle_timer_callback(int fd, short event, void* null) {
	struct timeval tv;
	size_t i, l = rand() % 100;
	tv.tv_sec = 0;
	tv.tv_usec = rand() % 1000000;
	evtimer_add(&timer, &tv); 
	for (i = 0; i < l; i++)
		async_syslog(log_state, LOG_ERR, "test nr %d", i);
}

int main(int argc, char *argv[]) {
	srand(0);
	base = (struct event_base*) event_init();
	log_state = async_openlog(base, "async_log_test", LOG_PID, LOG_DAEMON);
	evtimer_set(&timer, cycle_timer_callback, base);
	cycle_timer_callback(-1, 0, NULL);
	event_base_dispatch(base);
	return 0;
}
