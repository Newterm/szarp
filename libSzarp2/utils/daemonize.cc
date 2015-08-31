/**
 * System daemon tools for GNU/Linux.
 *
 * Copyright (C) 2015 Newterm
 * Author: Tomasz Pieczerak <tph@newterm.pl>
 *
 * This file is a part of SZARP SCADA software.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef MINGW32

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "daemonize.h"

/**
 * _signal() - a handy wrapper for changing signal actions.
 *
 * It's meant to be as simple as the deprecated signal(2) and as portable as
 * sigaction(2). Why signal(2) should be avoided - see its manpage.
 */
typedef void sigfunc_t(int);

sigfunc_t *_signal (int signo, sigfunc_t *func)
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (signo == SIGALRM) {
#ifdef  SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;   /* SunOS 4.x */
#endif
	}
	else {
#ifdef  SA_RESTART
		act.sa_flags |= SA_RESTART;     /* SVR4, 44BSD */
#endif
	}
	if (sigaction(signo, &act, &oact) < 0)
		return SIG_ERR;

	return oact.sa_handler;
}

/**
 * daemonize() - became a system daemon, gain control.
 *
 * The calling process is detached from the controlling terminal and made run
 * in the background.
 *
 * Returns zero on success. If an error occurs, returns -1 and sets errno to
 * any of the errors specified for the fork(2), setsid(2), sigaction(2),
 * chdir(2), open(2) and dup2(2).
 */
int daemonize (unsigned int flags)
{
	int i, fd;
	pid_t pid;

	/* first fork */
	if ( (pid = fork()) == -1)
		return -1;
	if (pid != 0)
		exit(0);	/* parent process */

	/* create a new session */
	setsid();

	/* set up signal handlers */
	if (_signal(SIGHUP, SIG_IGN) == SIG_ERR)
		return -1;

	if (_signal(SIGCHLD, SIG_DFL) == SIG_ERR)
		return -1;

	if (flags & DMN_SIGPIPE_IGN) {
		if (_signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			return -1;
	}

	/* second fork, give up session leadership */
	if ( (pid = fork()) == -1)
		return -1;
	if (pid != 0)
		exit(0);	/* parent process */

	/* change working directory to the root directory */
	if (!(flags & DMN_NOCHDIR)) {
		if (chdir("/") == -1)
			return -1;
	}

	/* reset umask */
	umask(0);

	/* redirect stdout, stdin and stderr to /dev/null */
	if (!(flags & DMN_NOREDIR_STD)) {
		close(0);	/* stdout */
		close(1);	/* stdin  */
		close(2);	/* stderr */

		if ( (fd = open("/dev/null", O_RDWR)) == -1)
			return -1;

		if ( (fd = dup2(fd, 0)) == -1)
			return -1;
		if ( (fd = dup2(fd, 1)) == -1)
			return -1;
		if ( (fd = dup2(fd, 2)) == -1)
			return -1;
	}

	return 0;	/* done */
}

#endif  /* MINGW32 */
