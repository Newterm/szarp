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

import sys
sys.path.append("/opt/szarp/lib/python")

import meaner
from libpar import LibparReader

if __name__ == "__main__":
	lpr = LibparReader()

	path = lpr.get("global", "szbase")
	ipk = lpr.get("global", "IPK")
	uri = lpr.get("global", "parcook_socket_uri")

	meaner = meaner.Meaner(path, uri)
	meaner.configure(ipk)

	meaner.run()

