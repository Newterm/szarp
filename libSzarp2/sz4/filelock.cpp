#include "config.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include "sz4/filelock.h"

namespace sz4 {

// struct flock
// 	.l_type   = { F_RDLCK, F_WRLCK, F_UNLCK };
//	.l_whence = { SEEK_SET, SEEK_CUR, SEEK_END };
//	.l_start  = [ offset from l_whence (int) ];
//	.l_len    = [ length (int), 0 = to EOF ];
//	.l_pid    = [ PID ];

int open_readlock (const char* pathname, int flags)
{
	int fd;
#ifndef MINGW32
	struct flock fl = {F_RDLCK, SEEK_SET, 0, 0, 0};
	fl.l_pid = getpid();
#endif

	if (-1 == (fd = open(pathname, flags)))
		throw file_open_error("Failed to open file: " + std::string(pathname));

#ifndef MINGW32
	if (-1 == (fcntl(fd, F_SETLKW, &fl))) {
		close(fd);
		throw file_lock_error("Failed to lock file: " + std::string(pathname));
	}
#endif

	return fd;
}

int open_writelock (const char* pathname, int flags, mode_t mode)
{
	int fd;
#ifndef MINGW32
	struct flock fl = {F_WRLCK, SEEK_SET, 0, 0, 0};
	fl.l_pid = getpid();
#endif

	if (-1 == (fd = open(pathname, flags, mode)))
		throw file_open_error("Failed to open file: " + std::string(pathname));

#ifndef MINGW32
	if (-1 == (fcntl(fd, F_SETLKW, &fl))) {
		close(fd);
		throw file_lock_error("Failed to lock file: " + std::string(pathname));
	}
#endif

	return fd;
}

int close_unlock (int fd)
{
#ifndef MINGW32
	struct flock fl = {F_UNLCK, SEEK_SET, 0 , 0, 0};

	if (-1 == fcntl(fd, F_SETLKW, &fl))
		throw file_lock_error("Failed to unlock file.");
#endif

	return close(fd);
}

int upgrade_lock (int fd)
{
#ifndef MINGW32
	struct flock fl = {F_WRLCK, SEEK_SET, 0 , 0, 0};
	fl.l_pid = getpid();

	if (-1 == (fcntl(fd, F_SETLKW, &fl)))
		throw file_lock_error("Failed to upgrade lock file.");
#endif
	return 0;
}

}

