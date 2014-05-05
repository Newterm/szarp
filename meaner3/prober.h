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
 * prober - daemon for writing 10-seconds data probes to disk
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#ifndef __PROBER_H__
#define __PROBER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <assert.h>
#include <errno.h>
#include <signal.h>

#include "liblog.h"
#include "libpar.h"
#include "execute.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"

#include "tprober.h"
#include "tparcook.h"

/* This macro decides how we exit from program. Normally, we exit by setting
 * default handler for termination signal and raising it, so correct return
 * status is set. But raising signal also terminates memory debuggers such as 
 * valgrind. If this macro is defined, we use 'exit()' function instead of
 * raising signal, so debbuger can continue and check for leaks (for example).
 */
#define VALGRIND_IN_USE 1

/** Path to main config file. */
#define SZARP_CFG "/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg"

/** Name of section in main config file. */
#define SZARP_CFG_SECTION "prober"

/** base period in seconds */
#define BASE_PERIOD	10

/***********************************************************************/

/* GLOBAL VARIABLES */

/** atomic flag for signal handling, set while writing to base is performed */
extern volatile sig_atomic_t g_signals_blocked;
/** atomic flag for signal handling, set if termination signal was cought
 * during writing to base and we should terminate */
extern volatile sig_atomic_t g_should_exit;

class TProber;
/** pointer to main program object, must be global because we use it in signal
 * handling (for cleanup) */
extern TProber* g_prober;

/** Handler for critical signals. Must be global, no cleanup is made, just
 * writing to log. */
RETSIGTYPE g_CriticalHandler(int signum);

/** Handler for terminations signals. Can be called with signum '0' to
 * force program exit. */
RETSIGTYPE g_TerminateHandler(int signum);

#endif
