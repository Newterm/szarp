""" SZARP: SCADA software 
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

			pos = self.file.tell()

			self.file.seek(0, 0)

			self.buf = cStringIO.StringIO()
			self.buf.write(self.file.read())

			self.buf.seek(pos, 0)

			self.dirty = len(self.buf.getvalue())

		def seek(self, offset, whence):
			self.buf.seek(offset, whence)

		def tell(self):
			return self.buf.tell()

		def write(self, data):
			self.dirty = min(self.dirty, self.buf.tell())
			self.buf.write(data)

		def read(self, size=-1):
			return self.buf.read(size)

		def __sync(self):
			self.file.seek(self.dirty, 0)
			self.file.write(self.buf.getvalue()[self.dirty:])
			self.dirty = self.file.tell()
			self.file.truncate(len(self.buf.getvalue()))

		def sync(self):
			self.__sync();

		def close(self, sync=True):
			if sync:
				self.__sync()
			self.file.close()

		def truncate(self, size):
			self.dirty = min(self.dirty, size)
			self.buf.truncate(size)

	def open(self, path, mode):
		return self.File(path, mode)

class SaveParam:
	def __init__(self, param, szbase_dir, file_factory=FileFactory()):
		self.param_path = parampath.ParamPath(param, szbase_dir)
		self.last_entry = lastentry.LastEntry(param)
		self.data_file_size = config.DATA_FILE_SIZE
		self.file_factory = file_factory
		self.file_nanotime = None
		self.file_time = None
		self.param = param
		self.file = None
		self.prev = None

		self.first_write = True

	def __read_bzip_file(self, path):
		file = self.file_factory.open(path, "r+b")
		try:
			io = cStringIO.StringIO()
			io.write(bz2.decompress(file.read()))
			io.seek(0, 0)
			return io
		finally:
			file.close(sync=False)

	def __create_current_file(self, value, time, nanotime):
		if self.file is not None:
			self.file.close(sync=False)

		self.file = self.file_factory.open(self.param_path.create_file_path(*self.param.max_time), "w+b")

		time_blob = self.param.time_to_binary(time, nanotime)
		self.file.write(time_blob)

		value_blob = self.param.value_to_binary(value)
		self.file.write(value_blob)

		self.file_size = len(time_blob) + len(value_blob)


	def __do_first_write_to_existing_file(self, value, time, nanotime):
		self.first_write = False

		self.file = self.file_factory.open(self.param_path.create_file_path(*self.param.max_time), "a+b")
		
		self.last_entry.from_current_file(self.file)

		self.file_size = self.file.tell()

		time_1, nanotime_1 = self.param.time_just_after(*self.last_entry.last_time())
		if self.param.is_time_before(time_1, nanotime_1, time, nanotime):
			self.__write_value(self.param.nan(), time_1, nanotime_1)

		self.__write_value(value, time, nanotime)

	def __do_first_write_last_present(self, value, time, nanotime):

		prev_entry = lastentry.LastEntry(self.param)
		vals = prev_entry.from_file(self.__read_bzip_file(self.prev), *self.param_path.time_from_path(self.prev))

		self.file = self.file_factory.open(self.param_path.create_file_path(*self.param.max_time), "w+b")

		last_time, last_nanotime = prev_entry.last_time()

		last_value_has_no_duration = len(vals) > 0 and vals[-1]['time'] == (last_time, last_nanotime)
		if last_value_has_no_duration == True:
			last_time, last_nanotime = self.param.time_just_after(last_time, last_nanotime)

		if self.param.is_time_before(last_time, last_nanotime, time, nanotime):
			self.__create_current_file(self.param.nan(), last_time, last_nanotime)
			self.last_entry.reset(last_time, last_nanotime, self.param.nan())
			self.__write_value(value, time, nanotime)

		elif (last_time, last_nanotime) == (time, nanotime):
			self.__create_current_file(value, time, nanotime)
			self.last_entry.reset(time, nanotime, value)

		else:
			raise lastentry.TimeError((last_time, last_nanotime), (time, nanotime))

		self.first_write = False

	def __do_first_write_to_new_file(self, value, time, nanotime):

		if self.prev is not None:
			self.__do_first_write_last_present(value, time, nanotime)
		else:

			param_dir = self.param_path.param_dir()
			if not os.path.exists(param_dir):
				os.makedirs(param_dir)

			self.__create_current_file(value, time, nanotime)
			self.last_entry.reset(time, nanotime, value)

			self.first_write = False


	def __do_first_write(self, value, time, nanotime):
		current, self.prev = self.param_path.find_latest_paths()

		if current is not None:
			self.__do_first_write_to_existing_file(value, time, nanotime)
		else:
			self.__do_first_write_to_new_file(value, time, nanotime)

	def __write_value(self, value, time, nanotime):
		if self.param.max_file_item_size + self.file_size > self.data_file_size:
			self.__start_next_file(value, time, nanotime)
		else:

			time_blob = self.last_entry.update_time(time, nanotime)
			self.file.write(time_blob)

			value_blob = self.param.value_to_binary(value)
			self.file.write(value_blob)

			self.file_size += len(time_blob) + len(value_blob)

	def __save_values(self, buf, entry, values):
		for v in values:
			if v['value'] != entry.value:
				if entry.value is not None:
					buf.write(entry.update_time(*v['time']))
					buf.write(self.param.value_to_binary(v['value']))
				else:
					buf.write(self.param.value_to_binary(v['value']))
				entry.value = v['value']
				

	def __try_appending_to_prev_file(self, vals):
		if self.prev is None:
			return False

		io = self.__read_bzip_file(self.prev)

		entry = lastentry.LastEntry(self.param)
		ovals = entry.from_file(io, *self.param_path.time_from_path(self.prev))

		#if the file is not empty...
		if entry.value is not None:
			io.seek(-entry.time_size, os.SEEK_END)
			self.__save_values(io, entry, vals)

			bzip2 = bz2.compress(io.getvalue())
			if len(bzip2) < self.data_file_size:
				self.__save_file(bzip2, self.prev)
				return True

		return False

	def __create_new_bz_file(self, vals):
		io = cStringIO.StringIO()

		entry = lastentry.LastEntry(self.param)
		entry.reset(*vals[0]['time'])
		self.__save_values(io, entry, vals)

		self.prev = self.param_path.create_file_path(*vals[0]['time'])
		self.__save_file(bz2.compress(io.getvalue()), self.prev)

	def __start_next_file(self, value, time, nanotime):
		vals = self.last_entry.from_current_file(self.file)

		if self.__try_appending_to_prev_file(vals) == False:
			self.__create_new_bz_file(vals)

		last_time, last_nanotime = self.param.time_just_after(*vals[-1]['time'])
		if self.param.is_time_before(last_time, last_nanotime, time, nanotime):
			self.__create_current_file(vals[-1]['value'], last_time, last_nanotime)
			self.last_entry.from_current_file(self.file)

			self.__write_value(value, time, nanotime)
		else:
			self.__create_current_file(value, time, nanotime)
			self.last_entry.from_current_file(self.file)


	def __save_file(self, data, path):
		temp = tempfile.NamedTemporaryFile(dir=self.param_path.param_dir(), delete=False)
		temp.write(data)
		temp.close()

		os.chmod(temp.name, 0644)
		os.rename(temp.name, path)

	def __process_value(self, value, time, nanotime = 0):
		if not self.param.written_to_base:
			return

		try:
			if not self.first_write:
				self.__write_value(value, time, nanotime)
			else:
				if self.param.isnan(value):
					return

				self.__do_first_write(value, time, nanotime)

		except lastentry.TimeError, e:
			syslog.syslog(syslog.LOG_WARNING,
					"Ignoring value for param %s, as this value (time:%s) is no later than lastest value(time:%s)" \
					% (self.param_path.param_path, e.current_time, e.msg_time))

	def process_value(self, value, time, nanotime = 0):
		self.__process_value(value, time, nanotime)
		if self.file is not None:
			self.file.sync()

	def process_msgs(self, msgs):
		if len(msgs) == 0:
			return;

		v = self.param.value_from_msg(msgs[0])
		self.__process_value(v, msgs[0].time, msgs[0].nanotime)

		for m in msgs[1:-1]:
			vn = self.param.value_from_msg(m)
			if vn != v:
				self.__process_value(self.param.value_from_msg(m), m.time, m.nanotime)
				v = vn

		if len(msgs) > 1:
			self.__process_value(self.param.value_from_msg(msgs[-1]), msgs[-1].time, msgs[-1].nanotime)

		if self.file is not None:
			self.file.sync()

		return msgs[-1].time

	def reset(self):
		self.close()
		self.first_write = True

	def close(self):
		if self.file is not None:
			self.file.close(sync=True)
			self.file = None
