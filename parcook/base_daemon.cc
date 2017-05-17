/* 
 * SZARP: SCADA software 
 *
 * Copyright (C) 
 * 2011 - Jakub Kotur <qba@newterm.pl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */
/** 
 * @file basedmn.cc
 * @brief Implementatio of BaseDaemon class
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.1
 * @date 2011-06-16
 */

#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include <liblog.h>

#include "base_daemon.h"

namespace BaseDaemonHelper {
RETSIGTYPE terminate_handler(int signum)
{
	sz_log(2, "signal %d caught, exiting", signum);
	signal(signum, SIG_DFL);
	raise(signum);
}
} // BaseDaemonHelper
