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

import os
import time
from datetime import datetime

from meanerbase import MeanerBase
from ipk import IPK

class Sz4Writer(MeanerBase):
	def __init__(self, path):
		MeanerBase.__init__(self, path)

	def configure(self, ipk_path):
		MeanerBase.configure(self, ipk_path)

		self.name2index = { sp.param.param_name : i for (i, sp) in enumerate(self.save_params) }

	def prepare_value(self, value, param):
		return float(value) * (10 ** param.prec)

	def process_file(self, path, time_format):
		f = open(path, 'r')
		
		for line in f:
			(pname, time_string, value_string) = line.rsplit(';', 3)

			pindex = self.name2index[unicode(pname, 'utf-8')]
			if pindex is None:
				continue
			sparam = self.save_params[pindex]

			_datetime = datetime.strptime(time_string, time_format)
			time_sec = time.mktime(_datetime.timetuple())
			time_nanosec = _datetime.time().microsecond * 1000

			value = self.prepare_value(value_string, sparam.param)
			sparam.process_value(value, time_sec, time_nanosec)

if __name__ == "__main__":
	writer = Sz4Writer(sys.argv[1] + '/sz4')
	writer.configure(sys.argv[1] + '/config/params.xml')

	writer.process_file(sys.argv[2], "%Y-%m-%d %H:%M:%S" if len(sys.argv) == 3 else sys.argv[3])

