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

import struct
import os
import math

class Param:
	def __init__(self, name, data_type, prec, time_prec, written_to_base):
		self.param_name = name
		self.data_type = data_type
		self.prec = prec
		self.time_prec = time_prec

		if self.data_type == "short":
			self.value_format_string = "<h"
			self.value_lenght = 2
			self.value_from_msg = lambda x : x.int_value
			self.isnan = lambda x : x == -2 ** 15
			self.nan = lambda : -2 ** 15
		elif self.data_type == "int":
			self.value_format_string = "<i"
			self.value_lenght = 4
			self.value_from_msg = lambda x : x.int_value
			self.isnan = lambda x : x == -2 ** 31
			self.nan = lambda : -2 ** 31
		elif self.data_type == "float":
			self.value_format_string = "<f"
			self.value_lenght = 4
			self.value_from_msg = lambda x : x.float_value
			self.isnan = lambda x : math.isnan(x)
			self.nan = lambda : float('nan')
		elif self.data_type == "double":
			self.value_format_string = "<d"
			self.value_lenght = 8
			self.value_from_msg = lambda x : x.double_value
			self.isnan = lambda x : math.isnan(x)
			self.nan = lambda : float('nan')
		else:
			raise ValueError("Unsupported value type: %s" % (self.data_type,))

		self.written_to_base = written_to_base
		self.max_file_item_size = self.value_lenght + self.time_prec + 1

		if time_prec == 8:
			self.max_time = 2 ** 32, 2 ** 32 - 1
		else:
			self.max_time = 2 ** 32 - 1, 0


	def value_to_binary(self, value):
		return struct.pack(self.value_format_string, value)

	def time_to_binary(self, time, nanotime):
		if self.time_prec == 8:
			return struct.pack("<II", time, nanotime)
		else:
			return struct.pack("<I", time)

	def value_from_binary(self, blob):
		return struct.unpack(self.value_format_string, blob)[0]

	def time_to_file_stamp(self, time, nanotime):
		if self.time_prec == 8:
			return time + float(nanotime) / (10 ** 9)

	def is_max_time(self, time, nanotime):
		if self.time_prec == 8:
			return time == self.max_time[0] and nanotime == self.max_time[1]
		else:
			return time == self.max_time[0]

	def is_time_before(self, l_time, l_nanotime, r_time, r_nanotime):
		return l_time < r_time \
			or self.time_prec == 8 \
				and l_time == r_time \
				and l_nanotime < r_nanotime

	def time_just_after(self, time, nanotime):
		if self.time_prec == 8:
			if nanotime == 10 ** 9 - 1:
				return time + 1, 0
			else:
				return time, nanotime + 1
		else:
			return time + 1, 0

	def time_just_before(self, time, nanotime):
		if self.time_prec == 8:
			if nanotime == 0:
				return time - 1, 10 ** 9 - 1
			else:
				return time, nanotime - 1
		else:
			return time - 1, 0

def from_node(node):
	param_name = node.attrib["name"]

	written_to_base = True if "base_ind" in node.attrib else False

	if "data_type" in node.attrib:
		data_type = node.attrib["data_type"]
	else:
		data_type = "short"

	if "prec" not in node.attrib:
		prec = 0
	else:
		prec = int(node.attrib["prec"])

	if "time_type" in node.attrib:
		time_prec = 8 if node.attrib["time_type"] == "nanosecond" else 4
	else:
		time_prec = 4

	return Param(param_name, data_type, prec, time_prec, written_to_base)
