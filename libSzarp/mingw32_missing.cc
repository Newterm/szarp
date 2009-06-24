/*
 * $Id$
 *

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * This file contains functions that are in missing in mingw32 standard
 * C library.
 * 
 * The mingw32_missing library is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with minw32_missing library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA.  
 */

#include "mingw32_missing.h"

#ifndef HAVE_SCANDIR

/* 
 * Copied from glibs sources and modified by me in May 2004.
 * Copyright  1992-1998, 2000, 2002 Free Software Foundation, Inc.
 */ 

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef SCANDIR
#define SCANDIR scandir
#define READDIR readdir
#define DIRENT_TYPE struct dirent
#endif

#ifdef _DIRENT_HAVE_D_NAMLEN
# define _D_EXACT_NAMLEN(d) ((d)->d_namlen)
# define _D_ALLOC_NAMLEN(d) (_D_EXACT_NAMLEN (d) + 1)
#else
# define _D_EXACT_NAMLEN(d) (strlen ((d)->d_name))
# ifdef _DIRENT_HAVE_D_RECLEN
#  define _D_ALLOC_NAMLEN(d) (((char *) (d) + (d)->d_reclen) - &(d)->d_name[0])
# else
#  define _D_ALLOC_NAMLEN(d) (sizeof (d)->d_name > 1 ? sizeof (d)->d_name : \
		                              _D_EXACT_NAMLEN (d) + 1)
# endif
#endif

#define D_ALLOC_NAMLEN(d) _D_ALLOC_NAMLEN(d)

int
SCANDIR (
     const char *dir,
     DIRENT_TYPE ***namelist,
     int (*select) (const DIRENT_TYPE *),
     int (*cmp) (const void *, const void *))
{
  DIR *dp = opendir (dir);
  DIRENT_TYPE **v = NULL;
  size_t vsize = 0, i;
  DIRENT_TYPE *d;
  int save;

  if (dp == NULL)
    return -1;

  save = errno;
  errno = 0;

  i = 0;
  while ((d = READDIR (dp)) != NULL)
    if (select == NULL || (*select) (d))
      {
	DIRENT_TYPE *vnew;
	size_t dsize;

	/* Ignore errors from select or readdir */
	errno = 0;

	if (i == vsize)
	  {
	    DIRENT_TYPE **_new;
	    if (vsize == 0)
	      vsize = 10;
	    else
	      vsize *= 2;
	    _new = (DIRENT_TYPE **) realloc (v, vsize * sizeof (*v));
	    if (_new == NULL)
	      break;
	    v = _new;
	  }

	dsize = &d->d_name[D_ALLOC_NAMLEN (d)] - (char *) d;
	//dsize = &d->d_name[strlen(d->d_name)] - (char *) d;
	vnew = (DIRENT_TYPE *) malloc (dsize);
	if (vnew == NULL)
	  break;

	v[i++] = (DIRENT_TYPE *) memcpy (vnew, d, dsize);
      }

  if ( errno != 0)
    {
      save = errno;

      while (i > 0)
	free (v[--i]);
      free (v);

      i = -1;
    }
  else
    {
      /* Sort the list if we have a comparison function to sort with.  */
      if (cmp != NULL)
	qsort (v, i, sizeof (*v), cmp);

      *namelist = v;
    }

  (void) closedir (dp);
  errno = save;

  return i;
}

#endif /* HAVE_SCANDIR */

#ifndef HAVE_ALPHASORT

/* 
 * Copied from glibs sources.
 * Copyright  1992, 1997, 1998 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 */
#include <dirent.h>
#include <string.h>

int
alphasort (const void *a, const void *b)
{
  return strcoll ((*(const struct dirent **) a)->d_name,
		  (*(const struct dirent **) b)->d_name);
}

#endif /* HAVE_ALPHASORT */

#ifndef HAVE_ASPRINTF

#include <stdarg.h>
#include <stdio.h>

int vasprintf (
     char **result_ptr,
     const char *format,
     va_list args)
{
  char *string = NULL;
  char *ret;
  
  int needed;
  size_t allocated = 50;
  
  do {
	allocated *= 2;

	char *t = (char *) realloc (string, allocated);
	if (t == NULL) {
		free(string);
		return -1;
	}
	string = t;

	needed = vsnprintf(string, allocated, format, args);

  } while (needed < 0 || needed == allocated);
  
  /* change size of memory */
  ret = (char*) realloc(string, needed + 1);
  if (ret == NULL) {
	  free(string);
	  return -1;
  }

  *result_ptr = ret;
  return needed;
}


/* 
 * Copied from glibs sources and modified on May 2004.
 * Copyright  1992, 1997, 1998 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 */

/* Write formatted output from FORMAT to a string which is
   allocated with malloc and stored in *STRING_PTR.  */
/* VARARGS2 */
int asprintf (char **string_ptr, const char *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = vasprintf (string_ptr, format, arg);
  va_end (arg);

  return done;
}

#endif /* HAVE_ASPRINTF */

#ifndef HAVE_TIMEGM

#include <time.h>
#include <stdlib.h>

/* 
 * http://lists.debian.org/deity/2002/04/msg00082.html 
 *
 

 * Converts struct tm to time_t, assuming the data in tm is UTC rather
 * than local timezone.
 * 
 * mktime is similar but assumes struct tm, also known as the
 * "broken-down" form of time, is in local time zone.  mktime_from_utc
 * uses mktime to make the conversion understanding that an offset
 * will be introduced by the local time assumption.
 * 
 * timegm then measures the introduced offset by applying
 * gmtime to the initial result and applying mktime to the resulting
 * "broken-down" form.  The difference between the two mktime results
 * is the measured offset which is then subtracted from the initial
 * mktime result to yield a calendar time which is the value returned.
 * 
 * tm_isdst in struct tm is set to 0 to force mktime to introduce a
 * consistent offset (the non DST offset) since tm and tm+o might be
 * on opposite sides of a DST change.
 * 
 * Some implementations of mktime return -1 for the nonexistent
 * localtime hour at the beginning of DST.  In this event, use
 * mktime(tm - 1hr) + 3600.
 * 
 * Schematically
 * mktime(tm)   --> t+o
 * gmtime(t+o)  --> tm+o
 * mktime(tm+o) --> t+2o
 * t+o - (t+2o - t+o) = t
 * 
 * Note that glibc contains a function of the same purpose named
 * `timegm' (reverse of gmtime).  But obviously, it is not universally
 * available, and unfortunately it is not straightforwardly
 * extractable for use here.  Perhaps configure should detect timegm
 * and use it where available.
 * 
 * Contributed by Roger Beeman <beeman@cisco.com>, with the help of
 * Mark Baushke <mdb@cisco.com> and the rest of the Gurus at CISCO.
 * Further improved by Roger with assistance from Edward J. Sabol
 * based on input by Jamie Zawinski.  
 *
 */


time_t timegm (struct tm *t)
{
	time_t tl, tb;
	struct tm *tg;
	
	/*this is required for consistency with libc timegm*/
	t->tm_isdst = -1;
	tl = mktime (t);
	if (tl == -1)
	{
		t->tm_hour--;
		tl = mktime (t);
		if (tl == -1)
			return -1; /* can't deal with output from strptime */
		tl += 3600;
	}
	tg = gmtime (&tl);
	tb = mktime (tg);
	if (tb == -1)
	{
		tg->tm_hour--;
		tb = mktime (tg);
		if (tb == -1)
			return -1; /* can't deal with output from gmtime */
		tb += 3600;
	}
	return (tl - (tb - tl));
}

#endif /* HAVE_TIMEGM */

