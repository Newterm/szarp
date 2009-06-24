/* 
  libSzarp - SZARP library

*/

/*
 * $Id$
 *
 * Pawe³ Pa³ucha
 * 19.06.2001
 * daemon.c
 *
 * libSzarp - biblioteka systemu SZARP 2.1
 *
 * Funkcja go_daemon - powoduje przejscie procesu w t³o, od³±cza go
 * od konsoli, zmienia katalog aktualny na "/".
 * UWAGA: zmienia PID procesu, ustawia umask na 0000 !
 *
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

void go_daemon(void)
{
 if (fork() > 0) exit(0);
 signal(SIGHUP, SIG_IGN);
 setsid();
 chdir("/");
 umask(0);
 signal(SIGCLD, SIG_DFL);

 signal(SIGPIPE, SIG_IGN);
 close(0); close(1); close(2);
 open("/dev/null", O_RDWR);
 dup2(0, 1); dup2(0, 2);
 umask(0002);
}

#endif /* MINGW32 */
