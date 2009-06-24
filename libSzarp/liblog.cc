/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * liblog.c
 *
 * Modu³ obs³ugi logowania zdarzeñ dla systemu SZARP.
 *
 * 2001.04.13 Pawel Palucha
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "liblog.h"

int __log_level = 0;
int __log_info = 1;
FILE * __logfd = NULL;

void sz_log_info(int info)
{
	__log_info = info;	
}

int sz_loginit(int level, const char * logfile)
{
    sz_logdone();
    
    if ( (level < 0 ) || (level > 10))
	__log_level = 0;
    else
	__log_level = level;
    
    if (logfile == NULL || strlen(logfile) == 0)
	return (__log_level);

    __logfd = fopen(logfile, "a");

    if (__logfd == NULL) {
	__logfd = stderr;
	return -1;
    }
    
    return (__log_level);
}

int sz_loginit_cmdline(int level, const char * logfile, int *argc, char *argv[])
{
	int i;
	int l;
	char *endptr, *endptr1;

	for (i = 1; i < *argc; i++) {
		if (!strncmp(argv[i], "--debug=", 8)) {
			l = strtol(argv[i]+8, &endptr, 10);
			if ((argv[i][8] == 0) || ((*endptr) != 0))
				goto fail;
			(*argc)--;
			if (i != *argc) {
				endptr = argv[i];
				memmove(argv+i, argv+i+1, (*argc-i) * 
						(sizeof(char *)));
				argv[*argc] = endptr;
			}
			return sz_loginit(l, logfile);
		}
		if (!strcmp(argv[i], "-d")) {
			if (*argc <= i+1)
				goto fail;	
			l = strtol(argv[i+1], &endptr, 10);
			if (*endptr != 0)
				goto fail;

			(*argc) -= 2;
			if (i != *argc) {
				endptr = argv[i];
				endptr1 = argv[i+1];
				memmove(argv+i, argv+i+2, (*argc-i) * 
						(sizeof(char *)));
				argv[*argc-1] = endptr;
				argv[*argc] = endptr1;
			}
			return sz_loginit(l, logfile);
		}
	}
fail:	
	return sz_loginit(level, logfile);
}

void sz_logdone(void)
{
    __log_level = 0;

    if ((__logfd != NULL) && (__logfd != stderr))
	fclose(__logfd);
    
    __logfd = NULL;
}

void vsz_log(int level, const char * msg_format, va_list fmt_args) {
    time_t i;
    char *timestr;
    
    if (level > __log_level)
	return;

    if (__logfd == NULL)
	__logfd = stderr;
    
    
    /* Troche kombinowania, coby wyswietlac nie cala date, tylko tak jak 
     * jest w /var/log/messages...
     */
    if (__log_info) {
	i = time(NULL);
	timestr = ctime(&i);
	timestr[19] = 0;
	fprintf(__logfd, "%s [%d] (%d) ", timestr+4, getpid(), level);
	timestr[19] = ' ';
    }
    
    vfprintf(__logfd, msg_format, fmt_args);
    fprintf(__logfd, "\n");
    fflush(__logfd);

}

void sz_log(int level, const char * msg_format, ...)
{
    if (level > __log_level)
	return;

    va_list fmt_args;
    va_start(fmt_args, msg_format);
    vsz_log(level, msg_format, fmt_args);
    va_end(fmt_args);
}
