/* 
  libSzarp - SZARP library

*/

/* $Id$ */

/*
 * SZARP 2.0
 * libSzarp
 * execute.c
 */

#include <config.h>
#ifndef MINGW32

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <liblog.h>

/****************************************************************************/
int execute(char *command)
/* odpowiednik system(3S) - zaczerpniête z Xlib Programming Manual p 551 */
/****************************************************************************/
{
 int		status, pid, w;
 void		(*istat)(int), (*qstat)(int);

 if ((pid = vfork()) == 0)
   {
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    execl("/bin/sh", "sh", "-c", command, NULL);
    _exit(127);
   }
 istat = signal(SIGINT, SIG_IGN);
 qstat = signal(SIGQUIT, SIG_IGN);
 while (((w = wait(&status)) != pid) && (w != 1))
   ;
 if (w == -1)
   status = -1;
 signal(SIGINT, istat);
 signal(SIGQUIT, qstat);
 return(status);
}

/*
 * execute_subst - execute specified command and returns it's output
 * (just like the shell `command` substitution). On error returns NULL.
 * Only the first 4 KB of command output are read. Uses strdup to allocate
 * memory.
 */
char * execute_subst(char *cmdline)
{
	/* input buffer */
#define INPUT_BUFFER_SIZE	4096
	char buffer[INPUT_BUFFER_SIZE], * retstr;
	/* bytes to read */
	int toread = INPUT_BUFFER_SIZE-1;
	/* bytes already read */
	int bread = 0;
	/* read() return code */
	int ret;	
	/* pipe */
	int pipe_fd[2];
	
	if (pipe (pipe_fd) < 0) {
	sz_log(1, "execute_subst: pipe() error, errno %d", errno);
			return NULL;
	}
sz_log(10, "pipe_fd = [%d, %d]", pipe_fd[0], pipe_fd[1]);
	switch (fork()) {
		case -1:
		sz_log(1, "execute_subst: fork() error, errno %d", errno);
				return NULL;
		case 0 :
		if (dup2(pipe_fd[1], 1) < 0) {
			sz_log(1, "execute_subst: dup2() error, errno %d",
					errno);
				return NULL;
			}
			execl("/bin/sh", "sh", "-c", cmdline, NULL);
		sz_log(1, "execute_subst: execl() error, errno %d", errno);
			return NULL;
		default:
			if (close(pipe_fd[1]) < 0) {
			sz_log(1, "execute_subst: close() error, errno %d",
					errno);
				return NULL;
			}

			while ((ret = read(pipe_fd[0], &(buffer[bread]), toread)) > 0) {
				bread += ret;
				toread -= ret;
			}
			buffer[bread] = 0;
			/* Read rest of the output to free the child */
			retstr = strdup(buffer);
			while (read(pipe_fd[0], buffer, INPUT_BUFFER_SIZE) > 0)
				;
			/* Wait for the child to exit */
			wait(NULL);
			close(pipe_fd[0]);
			return retstr;						
	}
}


/** 
 * Helper function for check_for_other(). Selects name of files that are
 * different from current pid and contains only digits.
 */
int select_proc_dirs (const char *d_name, const char *pid)
{
  if (!strcmp (pid, d_name))
    return 0;
  if (strspn (d_name, "0123456789") == strlen (d_name))
    return 1;
  return 0;
}

/**
 * Checks if other copy of currently executed program (with same program name
 * and same arguments) is already run.
 * @param argc main() functions argc param, read only
 * @param argv main() functions argv param, read only
 * @return 0 if this is the only copy, -1 if error occured while scanning proc
 * directory, otherwise returns pid of program's another copy (first found).
 */
int check_for_other (int argc, char *argv[])
{
  struct dirent **namelist;
  int n, l, i, on, ret;
  char cmdline[256] = "";
  char filename[256] = "";
  char buffer[256] = "";
  char pid_str[64];
  FILE *f;

  ret = 0;
  // our PID
  sprintf (pid_str, "%d", getpid ());
  // recreate command line
  n = 0;
  while (n < argc)
    sprintf (cmdline, "%s %s", cmdline, argv[n++]);
  // delete path from program's name
  l = 0;
  for (i = 1; (cmdline[i] != '\0') && (cmdline[i] != ' '); i++)
    if (cmdline[i] == '/')
      l = i;
  memmove (cmdline, cmdline + l + 1, strlen (cmdline) - l);
  // read proc dir content
  on = n = scandir ("/proc", &namelist, NULL, alphasort);
  if (n < 0) {
	  return -1;
  }
  while (n--) {
    if (!select_proc_dirs (namelist[n]->d_name, pid_str))
      continue;
    // name of file with process command line
    sprintf (filename, "/proc/%s/cmdline", namelist[n]->d_name);
    f = fopen (filename, "r");
    if (f) {
      // read the command line
      l = fread (buffer, 1, 256, f);
      fclose (f);
      // don't care about errors while reading single file
      if (l <= 0)
	continue;
      // merge params
      buffer[l] = 0;
      for (i = 0; i < l-1; i++)
	if (buffer[i] == '\0')
	  buffer[i] = ' ';
      // delete the path
      l = -1;
      for (i = 0; (buffer[i] != '\0') && (buffer[i] != ' '); i++)
	if (buffer[i] == '/')
	  l = i;
      memmove (buffer, buffer + l + 1, strlen (buffer) - l);
      // if this is the copy of same program, return its pid
      if (strcmp (buffer, cmdline) == 0) {
	ret = atoi (namelist[n]->d_name);
	goto at_exit;
      }
    }
  }
at_exit:
  for (i = 0; i < on; i++)
	  if (namelist[i])
		  free(namelist[i]);
  free(namelist);
  return ret;
}

#endif /* MINGW32 */
