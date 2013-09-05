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

static int __sz_loginit(int level, const char * logname)
{
    if ( (level < 0 ) || (level > 10))
	__log_level = 0;
    else
	__log_level = level;
    
    if (logname == NULL || strlen(logname) == 0)
	return (__log_level);

    __logfd = fopen(logname, "a");

    if (__logfd == NULL) {
	__logfd = stderr;
	return -1;
    }
    
    return (__log_level);
}

void* sz_log_classic_init(int level, const char* logname, SZ_LIBLOG_FACILITY facility, void* context) {
	__sz_loginit(level, logname);
	return NULL;
}

static void __sz_logdone(void)
{
    __log_level = 0;

    if ((__logfd != NULL) && (__logfd != stderr))
	fclose(__logfd);
    
    __logfd = NULL;
}

void sz_log_classic_close(void* data) {
	__sz_logdone();
}

static void __vsz_log(int level, const char * msg_format, va_list fmt_args) {
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

void sz_log_classic_vlog(void* data, int level, const char * msg_format, va_list fmt_args) {
	__vsz_log(level, msg_format, fmt_args);
}
