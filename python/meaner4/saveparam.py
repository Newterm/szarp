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
import bz2
import cStringIO
import tempfile
import syslog

class FileFactory:
	class File:
		def __init__(self, path, mode):
			self.file = open(path, mode, 0)

		def seek(self, offset, whence):
			self.file.seek(offset, whence)

		def tell(self):
			return self.file.tell()

		def write(self, data):
			self.file.write(data)

		def read(self):
			return self.file.read()

		def lock(self):
			fcntl.lockf(self.file, fcntl.LOCK_EX)

		def unlock(self):
			self.file.flush()
			fcntl.lockf(self.file, fcntl.LOCK_UN)

		def close(self):
			self.file.close()

		def truncate(self, size):
			self.file.truncate(size)

	def open(self, path, mode):
		return self.File(path, mode)

class SaveParam:
	def __init__(self, param, szbase_dir, file_factory=FileFactory()):
		self.param = param
		self.param_path = parampath.ParamPath(self.param, szbase_dir)
		self.first_write = True
		self.file_factory = file_factory
		self.last = lastentry.LastEntry(param)
		self.uncommited_last = lastentry.LastEntry(param)
		self.uncommited = None
		self.data_file_size = config.DATA_FILE_SIZE

	def update_last_time(self, time, nanotime):
		size = self.last.time_size
		blob = self.last.update_time(time, nanotime)

		self.file.seek(-size, os.SEEK_END)
		self.file.write(blob)

		size = self.uncommited_last.time_size
		blob = self.uncommited_last.update_time(time, nanotime)

		self.uncommited.seek(-size, os.SEEK_END)
		self.uncommited.write(blob)

			
	def write_value(self, value, time, nanotime):
		blob = self.param.value_to_binary(value)

		self.file.write(blob)
		self.last.new_value(time, nanotime, value)

		self.uncommited.write(blob)
		self.uncommited_last.new_value(time, nanotime, value)
		

	def prepare_for_writing(self, time, nanotime):
		path = self.param_path.find_latest_path()

		if path is not None:
			file = self.file_factory.open(path, "r+b")
			self.file = cStringIO.StringIO()
			self.file.write(bz2.decompress(file.read()))
			file.close()

			self.file_time, self.file_nanotime = self.param_path.time_from_path(path)
			self.last.from_file(self.file, self.file_time, self.file_nanotime)

			self.reset_uncommited()
		else:
			param_dir = self.param_path.param_dir()
			if not os.path.exists(param_dir):
				os.makedirs(param_dir)

			self.file = cStringIO.StringIO()
			self.uncommited = cStringIO.StringIO()

			self.file_time = time
			self.uncommited_time = time

			self.file_nanotime = nanotime
			self.uncommited_nanotime = nanotime


	def fill_no_data(self, time, nanotime):
		if not self.param.isnan(self.last.value):

			if self.last.time_size == 0:
				#set minimum duration for value at the end that didn't have the duration	
				#specified
				self.update_last_time(*self.param.time_just_after(*self.last.last_time()))

			value_blob = self.param.value_to_binary(self.param.nan())
			self.file.write(value_blob)
			self.uncommited.write(value_blob)

			time_blob = self.last.get_time_delta_since_latest_time(time, nanotime)
			self.file.write(time_blob)

			time_blob = self.uncommited_last.get_time_delta_since_latest_time(time, nanotime)
			self.uncommited.write(time_blob)

			self.last.reset(time, nanotime, self.param.nan())
			self.uncommited_last.reset(time, nanotime, self.param.nan())
		else:
			self.update_last_time(time, nanotime)

	def process_value(self, value, time, nanotime = 0):
		if not self.param.written_to_base:
			return

		try:
			if not self.first_write:
				self.update_last_time(time, nanotime)

				if value != self.last.value:
					self.write_value(value, time, nanotime)

			else:
				if self.param.isnan(value):
					return

				self.prepare_for_writing(time, nanotime)

				if self.last.value is not None:
					self.fill_no_data(time, nanotime)
				self.write_value(value, time, nanotime)

				self.first_write = False

		except lastentry.TimeError, e:
			syslog.syslog(syslog.LOG_WARNING,
					"Ignoring value for param %s, as this value (time:%s) is no later than lastest value(time:%s)" \
					% (self.param_path.param_path, e.current_time, e.msg_time))


	def process_msg(self, msg):
		self.process_value(self.param.value_from_msg(msg), msg.time, msg.nanotime)

	def process_msg_batch(self, batch):
		msg = batch[0]
		val = self.param.value_from_msg(msg)

		for nmsg in batch[1:]:
			nval = self.param.value_from_msg(nmsg)

			if nval != val:
				self.process_value(val, msg.time, msg.nanotime)
				val = nval

			msg = nmsg

		self.process_value(val, msg.time, msg.nanotime)

		self.commit()

		return msg.time

	def reset_uncommited(self):
		self.uncommited = cStringIO.StringIO()
		self.uncommited_time, self.uncommited_nanotime = self.last.last_time()

		self.uncommited_last.reset(self.uncommited_time, self.uncommited_nanotime, self.last.value)

		self.uncommited.write(self.param.value_to_binary(self.last.value))
		self.uncommited.write(self.uncommited_last.update_time(self.uncommited_time, self.uncommited_nanotime))

	def save_file(self, data, time, nanotime):
		temp = tempfile.NamedTemporaryFile(dir=self.param_path.param_dir(), delete=False)
		temp.write(data)
		temp.close()

		os.chmod(temp.name, 0644)
		os.rename(temp.name, self.param_path.create_file_path(time, nanotime))


	def commit(self):
		time = self.file_time
		nanotime = self.file_nanotime

		zipped = bz2.compress(self.file.getvalue())
		if len(zipped) >= self.data_file_size:
			time = self.uncommited_time 
			nanotime = self.uncommited_nanotime

			self.file_time = time
			self.file_nanotime = nanotime
			self.file = self.uncommited

			zipped = bz2.compress(self.uncommited.getvalue())

		self.save_file(zipped, time, nanotime)

		self.reset_uncommited()

			
	def close(self):
		if self.uncommited is not None:
			self.commit()
