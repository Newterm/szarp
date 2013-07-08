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

class Param:
	def __init__(self, node):
		self.param_name = node.attrib["name"]

		self.written_to_base = True if "base_ind" in node.attrib else False

		if "data_type" in node.attrib:
			self.data_type = node.attrib["data_type"]
		else:
			self.data_type = "short"

		if self.data_type == "short":
			self.value_format_string = "<h"
			self.value_lenght = 2
		elif self.data_type == "int": 
			self.value_format_string = "<i"
			self.value_lenght = 4
		elif self.data_type == "float": 
			self.value_format_string = "<f"
			self.value_lenght = 4
		elif self.data_type == "double": 
			self.value_format_string = "<d"
			self.value_lenght = 8
		else:
			raise ValueError("Unsupported value type: %s" % (self.data_type,))

		if "time_prec" in node.attrib:
			self.time_prec = int(node.attrib["time_prec"])
		else:
			self.time_prec = 4

	def value_to_binary(self, value):
		return struct.pack(self.value_format_string, value)

	def time_to_binary(self, time, nanotime):
		if self.time_prec == 8:
			return struct.pack("<II", time, nanotime)
		else:
			return struct.pack("<I", time)

	def value_from_msg(self, msg):
		if self.data_type == "short" or self.data_type == "int":
			return msg.int_value
		elif self.data_type == "float": 
			return msg.float_value
		elif self.data_type == "double": 
			return msg.double_value
			
	def value_from_binary(self, blob):
		return struct.unpack(self.value_format_string, blob)[0]

	def time_to_file_stamp(self, time, nanotime):
		if self.time_prec == 8:
			return time + float(nanotime) / (10 ** 9)
		else:
			return time


