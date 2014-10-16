#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/time.h>
#include <fcntl.h>
#include <vector>

inline struct timeval ms2timeval(time_t ms) {
	struct timeval tv;
	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
	return tv;
}

inline int set_fd_nonblock(int fd, bool nonblock = true) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		return -1;
	}

	if (nonblock) {
		flags |= O_NONBLOCK;
	} else {
		flags &= ~O_NONBLOCK;
	}

	flags = fcntl(fd, F_SETFL, flags);
	if (flags == -1) {
		return -1;
	}

	return 0;
}

void print_as_hex(const std::vector<unsigned char> &buf);

#endif
