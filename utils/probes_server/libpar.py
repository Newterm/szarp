"""
  Libpar config library access module.

  SZARP: SCADA software 
  Pawel Palucha <pawel@praterm.com.pl>

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

from subprocess import Popen, PIPE

class LibparReader:
	"""
	Class for reading parameters from szarp.cfg files.
	"""
	def __init__(self):
		pass

	def get(self, section, parameter):
		"""
		Return value of parameter from section, empty if parameter is not found.
		"""
		return Popen(["/opt/szarp/bin/lpparse", "-s", section, parameter],
				stdout=PIPE).communicate()[0].strip()



