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
#ifndef __DAEMONIZE_H
#define __DAEMONIZE_H

/* daemonize() flags */
#define DMN_SIGPIPE_IGN  0x01  /* set IGNORE action on SIGPIPE */
#define DMN_NOREDIR_STD  0x02  /* don't redirect std descriptors to /dev/null */
#define DMN_NOCHDIR      0x04  /* don't change the working directory to "/" */

int daemonize (unsigned int flags);

#endif  /* __DAEMONIZE_H */
