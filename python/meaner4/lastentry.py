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
import struct
import timedelta

class TimeError(Exception):
	pass

class LastEntry:
	def __init__(self, param):
		self.param = param
		self.delta_cache = {}

	def time_to_int(self, time, nanotime):
		if self.param.time_prec == 8:
			time = long(time) * 1000000000
			time += nanotime

		return time

	def reset(self, time, nanotime, value = None):
		self.time_size = 0
		self.time = self.time_to_int(time, nanotime)
		self.value_start_time = self.time
		self.value = value 

	def update_time(self, time, nanotime):
		time = self.time_to_int(time, nanotime)

		diff = time - self.value_start_time
		if diff <= 0:
			raise TimeError()

		self.time = time

		if diff in self.delta_cache:
			encoded = self.delta_cache[diff]
		else:
			encoded = timedelta.encode(diff)
			self.delta_cache[diff] = encoded

		self.time_size = len(encoded)

		return encoded

	def read_time(self, file):
		delta, self.time_size = timedelta.decode(file)
		return delta

	def new_value(self, time, nanotime, value):
		self.reset(time, nanotime, value)

	def from_file(self, file, time, nanotime):
		self.reset(time, nanotime)

		file.seek(0, 2)
		file_size = file.tell()
		file.seek(0, 0)

		while file.tell() < file_size:
			value = file.read(self.param.value_lenght)
			self.value = self.param.value_from_binary(value)

			self.value_start_time = self.time

			if file.tell() == file_size:
				self.time_size = 0
				break

			self.time += self.read_time(file)	

