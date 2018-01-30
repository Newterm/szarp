#!/usr/bin/python
"""
  SZARP: SCADA software 
  Darek Marcinkiewicz <reksio@newterm.pl>

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

"""

import fcntl, sys, os, struct
sys.path.append("/opt/szarp/lib/python")
import meaner
from libpar import LibparReader

lock_file = '/var/lock/meaner4.lock'

def go():
	lpr = LibparReader()

	path = lpr.get("", "sz4dir")
	ipk = lpr.get("", "IPK")
	uri = lpr.get("parhub", "pub_conn_addr")
	heartbeat = int(lpr.get("sz4", "heartbeat_frequency"))
	interval = int(lpr.get("sz4", "saving_interval"))

	m = meaner.Meaner(path, uri, heartbeat, interval)
	m.configure(ipk)

	m.run()

if __name__ == "__main__":
    process_id = os.getpid()
    """
	    struct flags {
		    ...
		    short l_type;    / Type of lock: F_RDLCK, F_WRLCK, F_UNLCK /
		    short l_whence;  / How to interpret l_start: SEEK_SET, SEEK_CUR, SEEK_END /
		    off_t l_start;   / Starting offset for lock /
		    off_t l_len;     / Number of bytes to lock /
		    pid_t l_pid;     / PID of process blocking our lock (set by F_GETLK and F_OFD_GETLK) /
		    ...
	    };
    """
    flags = struct.pack('hhllh', fcntl.F_WRLCK, os.SEEK_SET, 0, 100, process_id)
    with open(lock_file, 'w+') as fp:
        try:
            fcntl.fcntl(fp, fcntl.F_SETLK, flags)
            print("File is not locked, starting")
            go()
        except:
            print("Cannot set lock, exiting")
            exit(1) #TODO print pid number of process locking file
