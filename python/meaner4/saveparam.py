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
import math

class SaveParam:
	def __init__(self, param, szbase_dir):
		self.param = param
		self.param_path = parampath.ParamPath(self.param, szbase_dir)
		self.file = None
		self.current_value = None
		self.first_write = True

	def update_last_time(self, time, nanotime):
		self.file.seek(-self.param.time_prec, os.SEEK_END)
		try:
			fcntl.lockf(self.file, fcntl.LOCK_EX)
			self.file.write(self.param.time_to_binary(time, nanotime))
			self.file.flush()
		finally:
			fcntl.lockf(self.file, fcntl.LOCK_UN)

	def new_value(self, value, time, nanotime):
		if self.file_size + self.param.time_prec + self.param.value_lenght >= config.DATA_FILE_SIZE:
			self.file.close()

			path = self.param_path.create_file_path(time, nanotime)
			self.file = open(path, "w+b")
			self.file_size = 0

		try:
			fcntl.lockf(self.file, fcntl.LOCK_EX)
			self.file.write(self.param.value_to_binary(value))
			self.file.write(self.param.time_to_binary(time, nanotime))
			self.file.flush()
	
			self.file_size += self.param.time_prec + self.param.value_lenght
		finally:
			fcntl.lockf(self.file, fcntl.LOCK_UN)

		self.current_value = value

	def prepare_for_writing(self, time, nanotime):
		path = self.param_path.find_latest_path()	

		if path is not None:
			self.file_size = os.path.getsize(path)
			self.file = open(path, "r+b")

			if self.file_size >= self.param.time_prec + self.param.value_lenght:
				self.file.seek(-(self.param.time_prec + self.param.value_lenght), os.SEEK_END)
				value_blob = self.file.read(self.param.value_lenght)
				self.file.seek(0, os.SEEK_END)

				self.current_value = self.param.value_from_binary(value_blob)
			else:
				self.current_value = None
		else:
			self.file_size = 0

			param_dir = self.param_path.param_dir()
			if not os.path.exists(param_dir):
				os.makedirs(param_dir)

			path = self.param_path.create_file_path(time, nanotime)
			self.file = open(path, "w+b")
			self.current_value = None


	def process_msg(self, msg):
		if not self.param.written_to_base:
			return

		if self.first_write:
			self.prepare_for_writing(msg.time, msg.nanotime)
			self.first_write = False

		value = self.param.value_from_msg(msg)
		if  value == self.current_value:
			self.update_last_time(msg.time, msg.nanotime)
		else:
			self.new_value(value, msg.time, msg.nanotime)

