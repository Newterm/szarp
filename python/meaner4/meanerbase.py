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

import lxml
import lxml.etree
import param
import saveparam
from ipk import IPK

class MeanerBase:
	def __init__(self, path):
		self.save_params = []

		self.szbase_path = path

	def configure(self, ipk_path):
		self.ipk = IPK(ipk_path)
		
		for p in self.ipk.params:
			self.save_params.append(saveparam.SaveParam(p, self.szbase_path))

