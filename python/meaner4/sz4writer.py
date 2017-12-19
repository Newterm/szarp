#!/usr/bin/python
# -*- coding: utf-8 -*-
from __future__ import print_function

__license__ = \
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

__doc__ = \
"""
SZARP database writer.

Example of use:
		./sz4writer.py /opt/szarp/<prefix> /opt/szarp/<prefix>/<file.szw>

Default time format:
		"%Y-%m-%d %H:%M"

For more directives check out the python "datetime" documentation.
"""

import sys
sys.path.append("/opt/szarp/lib/python")
sys.path.append("/opt/szarp/lib/python/meaner4")
import os
import time
from datetime import datetime

from meanerbase import MeanerBase
from ipk import IPK
import unicodedata
import argparse
from argparse import RawTextHelpFormatter
import shlex

class Sz4Writer(MeanerBase):
	def __init__(self, path):
		MeanerBase.__init__(self, path)

	def configure(self, ipk_path):
		MeanerBase.configure(self, ipk_path)

		self.name2index = { sp.param.param_name : i for (i, sp) in enumerate(self.save_params) }

	def prepare_value(self, value, param):
		if "short" in param.data_type:
			if value:
				return int(value)
			else:
				return -2 ** 15

		elif "integer" in param.data_type:
			if value:
				return int(value)
			else:
				return -2 ** 31

		elif "float" in param.data_type:
			if value:
				return float(value)
			else:
				return float('nan')

		elif "double" in param.data_type:
			if value:
				return float(value)
			else:
				return float('nan')

	def process_szw_file(self, path, time_format):
		f = open(path, 'r')

		for line in f:
			spl = shlex.split(line)
			pname = spl[0]
			pname = pname.lstrip()
			time_string = spl[1] + "-" + spl[2] + "-" + spl[3] + " " + spl[4] + ":" + spl[5]
			value_string = spl[6]

			try:
				pindex = self.name2index[unicode(pname, 'utf-8')]
			except:
				print("Given parameter name: \"" + pname + "\" is not in the params.xml", file=sys.stderr)
				print("Can't save to szbase, aborting !", file=sys.stderr)
				exit(1)

			if pindex is None:
				continue
			sparam = self.save_params[pindex]

			_datetime = datetime.strptime(time_string, time_format)
			time_sec = time.mktime(_datetime.timetuple())
			time_nanosec = _datetime.time().microsecond * 1000

			value = self.prepare_value(value_string, sparam.param)
			sparam.process_value(value, int(time_sec), time_nanosec)

	def process_file(self, path, time_format):
		f = open(path, 'r')

		for line in f:
			(pname, time_string, value_string) = line.rsplit(';', 3)

			pindex = self.name2index[unicode(pname, 'utf-8').strip()]
			if pindex is None:
				continue
			sparam = self.save_params[pindex]

			_datetime = datetime.strptime(time_string.strip(), time_format)
			time_sec = time.mktime(_datetime.timetuple())
			time_nanosec = _datetime.time().microsecond * 1000

			value = self.prepare_value(value_string.strip().replace(',', '.'), sparam.param)
			sparam.process_value(value, int(time_sec), time_nanosec)

parser = argparse.ArgumentParser(description=__doc__, formatter_class=RawTextHelpFormatter)
parser.add_argument('path_to_base', help = 'path to base')
parser.add_argument('input_file', help = 'path to file which is going to be written as .sz4')
parser.add_argument('time_format', nargs='?', help = 'optional')
args = parser.parse_args()

if __name__ == "__main__":
	writer = Sz4Writer(args.path_to_base + '/szbase')
	writer.configure(args.path_to_base + '/config/params.xml')


	if ".szw" in args.input_file:
		writer.process_szw_file(args.input_file, "%Y-%m-%d %H:%M" if args.time_format == None else args.time_format)
	elif ".sz4" in args.input_file:
		writer.process_file(args.input_file, "%Y-%m-%d %H:%M:%S" if args.time_format == None else args.time_format)
	else:
		print("Wrong input file format! Aborting !", file=sys.stderr)
