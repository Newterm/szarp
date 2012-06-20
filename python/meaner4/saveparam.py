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

import os

import config
import param
import parampath
import math

class SaveParam:
	def __init__(self, node, szbase_dir):
		self.param = param.Param(node)
		self.param_path = parampath.ParamPath(self.param, szbase_dir)
		self.file = None
		self.last_value = None

	def open_latest_file(self, time, nanotime):
		path = self.param_path.find_latest_path()
		if path is None:
			try:
				os.makedirs(self.param_path.param_dir())
			except IOError:
				pass

			path = self.param_path.create_file_path(time, nanotime)

		self.file = open(path, "w+")

	def read_last_value(self):
		if os.fstat(self.file.fileno()).st_size == 0:
			return

		self.file.seek(-self.param.value_lenght, os.SEEK_END)
		self.last_value = self.param.value_from_string(self.file.read(sel.param.value_lenght))

	def update_last_value_time(self, msg):
		time = self.param.time_to_file_stamp(msg.time, msg.nanotime)
		os.utime(self.file.name, (time, time))

	def add_new_value(self, new_val, msg):
		if self.last_value is not None:
			mod_time = os.fstat(self.file.fileno()).st_mtime

			time = math.floor(mod_time)
			nanotime = (mod_time - time) * (10 ** 9)

			self.file.write(self.param.time_to_binary(time, nanotime))
			self.file.flush()

		if os.fstat(self.file.fileno()).st_size + \
				self.param.time_prec + \
				self.param.value_lenght \
				> config.DATA_FILE_SIZE:
			self.file.close()

		self.file = open(self.param_path.create_file_path(msg.time, msg.nanotime), "r+")

		self.file.write(self.param.value_to_binary(new_val))
		self.file.flush()

		self.update_last_value_time(msg)	

		self.last_value = new_val

	def process_msg(self, msg):
		if not self.param.written_to_base:
			return

		if self.file is None:
			self.open_latest_file(msg.time, msg.nanotime)

		if self.last_value is None:
			self.read_last_value()

		new_val = self.param.value_from_msg(msg)
		if  new_val == self.last_value:
			self.update_last_value_time(msg)
		else:
			self.add_new_value(new_val, msg)

		

