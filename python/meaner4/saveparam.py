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

class SaveFile:
	def __init__(self, path):
		self.path = path
		self.tmp_path = os.path.join(os.path.dirname(path), "meaner3.tmp")
		self.file = None

		if os.path.exists(path):
			st = os.stat(path)
			self.size = st.st_size
			self.time = math.floor(st.st_mtime)
			self.nanotime = (st.st_mtime - self.time) * (10 ** 9)
			self.exists = True
		else:
			self.size = 0
			self.time = None
			self.nanotime = None
			self.exists = False

	def write(self, value, time, nanotime):
		self.time = time
		self.nanotime = nanotime

		if self.exists:
			os.rename(self.path, self.tmp_path)

		with open(self.tmp_path, "a+") as f:
			f.write(value)

		os.utime(self.tmp_path, (time, time))
		os.rename(self.tmp_path, self.path)

		self.exists = True
		self.size += len(value)

	def update_mod_time(self, time, nanotime):
		self.time = time
		self.nanotime = nanotime

		if nanotime is not None:
			time = time + float(nanotime) / (10 ** 9)
		os.utime(self.path, (time, time))
		
	def close(self):
		if self.file is not None:
			self.file.close()


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

		self.file = SaveFile(path)

	def read_last_value(self):
		if not self.file.exists or self.file.size == 0:
			return

		with open(self.file.path, 'r') as f:
			f.seek(-self.param.value_lenght, os.SEEK_END)
			self.last_value = self.param.value_from_binary(f.read(self.param.value_lenght))

	def add_new_value(self, new_val, msg):
		new_val_blob = self.param.value_to_binary(new_val)
		if self.last_value is not None:
			old_time_blob = self.param.time_to_binary(self.file.time, self.file.nanotime)

			if self.file.size + 2 * self.param.time_prec + self.param.value_lenght > config.DATA_FILE_SIZE:
				self.file.write(old_time_blob, self.file.time, self.file.nanotime)
				self.file.close()

				self.file = SaveFile(self.param_path.create_file_path(msg.time, msg.nanotime))
				self.file.write(new_val_blob, msg.time, msg.nanotime)
			else:
				self.file.write(old_time_blob + new_val_blob, msg.time, msg.nanotime)
		else:
				self.file.write(new_val_blob, msg.time, msg.nanotime)
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
			self.file.update_mod_time(msg.time, msg.nanotime)
		else:
			self.add_new_value(new_val, msg)

		

