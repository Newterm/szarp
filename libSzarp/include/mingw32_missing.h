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

#ifndef __MINGW32_MISSING_H__
#define __MINGW32_MISSING_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_SCANDIR

#include <dirent.h>

#ifdef __cplusplus
extern "C"
#endif
int
scandir (
     const char *dir,
     struct dirent ***namelist,
     int (*select) (const struct dirent *),
     int (*cmp) (const void *, const void *));
#endif /* HAVE_SCANDIR */

#ifndef HAVE_ALPHASORT

#include <dirent.h>

#ifdef __cplusplus
extern "C"
#endif
int alphasort (const void *a, const void *b);

#endif /* HAVE_ALPHASORT */

#ifndef HAVE_ASPRINTF

#include <stdio.h>

#ifdef __cplusplus
extern "C"
#endif
int asprintf (char ** pointer, const char * format, ...);

#endif /* HAVE_ASPRINTF */

#ifndef HAVE_TIMEGM

#include <time.h>

#ifdef __cplusplus
extern "C"
#endif
time_t timegm (struct tm *tm);

#endif /* HAVE_TIMEGM */

#ifndef HAVE_POW10
#define pow10(x) pow(10,x)
#endif /* HAVE_POW10 */

#endif /* __MINGW32_MISSING_H__ */
