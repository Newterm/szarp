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

/*
 * $Id$
 *
 * Pawe³ Pa³ucha
 * 19.06.2001
 * daemon.c
 */

#include <config.h>
#ifndef MINGW32

#include <daemon.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <liblog.h>
#include <string.h>

int go_daemon(void)
{
	if (fork() > 0) exit(0);
	signal(SIGHUP, SIG_IGN);
	if (setsid() == -1) {
		sz_log(1, "setsid() failed, errno is %d (%s)", errno, strerror(errno));
		return -1;
	}
	chdir("/");
	umask(0);
	signal(SIGCLD, SIG_DFL);
	signal(SIGPIPE, SIG_IGN);
	close(0); close(1); close(2);
	int fd = open("/dev/null", O_RDWR);
	if (fd == -1) {
		sz_log(1, "open(\"/dev/null\") failed, errno is %d (%s)", errno, strerror(errno));
		return -1;
	}
	if (dup2(fd, 0) == -1) {
		sz_log(1, "dup2 failed for fd 0, errno is %d (%s)", errno, strerror(errno));
		return -1;
	}
	if (dup2(fd, 1) == -1) {
		sz_log(1, "dup2 failed for fd 1, errno is %d (%s)", errno, strerror(errno));
		return -1;
	}
	if (dup2(fd, 2) == -1) {
		sz_log(1, "dup2 failed for fd 2, errno is %d (%s)", errno, strerror(errno));
		return -1;
	}
	close(fd);
	umask(0002);
	return 0;
}

#endif /* MINGW32 */
