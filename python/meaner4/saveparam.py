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
import fcntl

import config
import param
import parampath
import lastentry
import math
from contextlib import contextmanager

@contextmanager
def file_lock(file):
	file.lock()
	yield file
	file.unlock()

class FileFactory:
	class File:
		def __init__(self, path, mode):
			self.file = open(path, mode)

		def seek(self, offset, whence):
			self.file.seek(offset, whence)

		def tell(self):
			return self.file.tell()

		def write(self, data):
			self.file.write(data)

		def read(self, len):
			return self.file.read(len)

		def lock(self):
			fcntl.lockf(self.file, fcntl.LOCK_EX)

		def unlock(self):
			self.file.flush()
			fcntl.lockf(self.file, fcntl.LOCK_UN)

		def close(self):
			self.file.close()

	def open(self, path, mode):
		return self.File(path, mode)

class SaveParam:
	def __init__(self, param, szbase_dir, file_factory=FileFactory()):
		self.param = param
		self.param_path = parampath.ParamPath(self.param, szbase_dir)
		self.file = None
		self.first_write = True
		self.file_factory = file_factory
		self.last = lastentry.LastEntry(param)

	def update_last_time_unlocked(self, time, nanotime):
		self.file.seek(-self.last.time_size, os.SEEK_END)

		last_time_size = self.last.time_size
		time_blob = self.last.update_time(time, nanotime)

		self.file.write(time_blob)

		self.file_size += self.last.time_size - last_time_size


	def update_last_time(self, time, nanotime):
		with file_lock(self.file):
			self.update_last_time_unlocked(time, nanotime)
			
	def ensure_room_for_new_value(self, time, nanotime):
		if self.file_size + self.param.value_lenght >= config.DATA_FILE_SIZE:
			self.file.close()

			path = self.param_path.create_file_path(time, nanotime)
			self.file = self.file_factory.open(path, mode="w+b")
			self.last.reset(time, nanotime)
			self.file_size = 0

	def write_value(self, value, time, nanotime):
		self.ensure_room_for_new_value(time, nanotime)

		with file_lock(self.file):
			self.file.write(self.param.value_to_binary(value))
			self.file_size += self.param.value_lenght

			self.last.new_value(time, nanotime, value)
			self.update_last_time_unlocked(time, nanotime)

	def prepare_for_writing(self, time, nanotime):
		path = self.param_path.find_latest_path()	

		if path is not None:
			self.file_size = os.path.getsize(path)
			self.file = self.file_factory.open(path, "r+b")

			file_time, file_nanotime = self.param_path.time_from_path(path)
			self.last.from_file(self.file, file_time, file_nanotime)
		else:
			self.file_size = 0

			param_dir = self.param_path.param_dir()
			if not os.path.exists(param_dir):
				os.makedirs(param_dir)

			path = self.param_path.create_file_path(time, nanotime)
			self.file = self.file_factory.open(path, "w+b")

			self.last.reset(time, nanotime)


	def fill_no_data(self, time, nanotime):
		if self.param.isnan(self.last.value):
			self.update_last_time(time, nanotime)
		else:
			self.ensure_room_for_new_value(time, nanotime)

			with file_lock(self.file):
				self.file.write(self.param.value_to_binary(self.param.nan()))
				self.file_size += self.param.value_lenght

				self.last.advance(time, nanotime, self.param.nan())
				self.update_last_time_unlocked(time, nanotime)

	def process_value(self, value, time, nanotime = 0):
		if not self.param.written_to_base:
			return

		if self.first_write:
			self.prepare_for_writing(time, nanotime)

			if self.last.value is not None:
				self.fill_no_data(time, nanotime)
			self.write_value(value, time, nanotime)

			self.first_write = False
		else:
			self.update_last_time(time, nanotime)
			if value != self.last.value:
				self.write_value(value, time, nanotime)


	def process_msg(self, msg):
		self.process_value(self.param.value_from_msg(msg), msg.time, msg.nanotime)

	def close(self):
		if self.file:
			self.file.close()
			self.file = None
				
