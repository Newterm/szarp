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

#ifndef __DAEMON_H__
#define __DAEMON_H__

/**
 * "Daemonize" current process.
 * Makes process go into background, detaches it from console, changes current working
 * dir to "/".
 * Warning: changes pid of process and sets umask to 0002.
 * @return 0 on success, -1, on error
 */
int go_daemon(void);

#endif
