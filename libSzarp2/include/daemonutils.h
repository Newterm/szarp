#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/time.h>
#include <fcntl.h>
#include <vector>
#include <termios.h>

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

inline speed_t speed_to_constant(unsigned int speed) {
	speed_t speed_constant = B0;
	switch (speed)
	{
	case 921600:
		speed_constant = B921600;
		break;
	case 460800:
		speed_constant = B460800;
		break;
	case 230400:
		speed_constant = B230400;
		break;
	case 115200:
		speed_constant = B115200;
		break;
	case 57600:
		speed_constant = B57600;
		break;
	case 38400:
		speed_constant = B38400;
		break;
	case 19200:
		speed_constant = B19200;
		break;
	case 9600:
		speed_constant = B9600;
		break;
	case 4800:
		speed_constant = B4800;
		break;
	case 2400:
		speed_constant = B2400;
		break;
	case 1800:
		speed_constant = B1800;
		break;
	case 1200:
		speed_constant = B1200;
		break;
	case 600:
		speed_constant = B600;
		break;
	case 300:
		speed_constant = B300;
		break;
	case 200:
		speed_constant = B200;
		break;
	case 150:
		speed_constant = B150;
		break;
	case 134:
		speed_constant = B134;
		break;
	case 110:
		speed_constant = B110;
		break;
	case 75:
		speed_constant = B75;
		break;
	case 50:
		speed_constant = B50;
		break;
	default:
		speed_constant = 0;
	}
	return speed_constant;
}

void print_as_hex(const std::vector<unsigned char> &buf);

#endif
