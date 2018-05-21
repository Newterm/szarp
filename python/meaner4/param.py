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

extra_ns = '{http://www.praterm.com.pl/SZARP/ipk-extra}'

def cast_value_to_int(pv, prec_adj):
	if pv.HasField('int_value'):
		return pv.int_value
	elif pv.HasField('float_value'):
		return int(pv.float_value * prec_adj)
	elif pv.HasField('double_value'):
		return int(pv.double_value * prec_adj)
	return None

def cast_value_to_double(pv, prec_adj):
	if pv.HasField('int_value'):
		return float(pv.int_value) / prec_adj
	elif pv.HasField('float_value'):
		return pv.float_value
	elif pv.HasField('double_value'):
		return pv.double_value
	return None

class Param:
	def __init__(self, name, data_type, prec, time_prec, written_to_base):
		self.param_name = name
		self.data_type = data_type
		self.prec = prec
		self.time_prec = time_prec

		if self.data_type == "short":
			self.value_format_string = "<h"
			self.value_lenght = 2
			self.value_from_msg = lambda x : cast_value_to_int(x, 10**self.prec)
			self.nan = lambda : -2 ** 15
			self.isnan = lambda x: x == self.nan
		elif self.data_type == "ushort":
			self.value_format_string = "<H"
			self.value_lenght = 2
			self.value_from_msg = lambda x : cast_value_to_int(x, 10**self.prec)
			self.nan = lambda : 2 ** 16 - 1
			self.isnan = lambda x: x == self.nan
		elif self.data_type == "integer":
			self.value_format_string = "<i"
			self.value_lenght = 4
			self.value_from_msg = lambda x : cast_value_to_int(x, 10**self.prec)
			self.nan = lambda : -2 ** 31
			self.isnan = lambda x: x == self.nan
		elif self.data_type == "uinteger":
			self.value_format_string = "<I"
			self.value_lenght = 4
			self.value_from_msg = lambda x : cast_value_to_int(x, 10**self.prec)
			self.nan = lambda : 2 ** 32 - 1
			self.isnan = lambda x: x == self.nan
		elif self.data_type == "float":
			self.value_format_string = "<f"
			self.value_lenght = 4
			self.value_from_msg = lambda x : cast_value_to_double(x, 10**self.prec)
			self.isnan = lambda x: math.isnan(x)
			self.nan = lambda : float('nan')
		elif self.data_type == "double": 
			self.value_format_string = "<d"
			self.value_lenght = 8
			self.value_from_msg = lambda x : cast_value_to_double(x, 10**self.prec)
			self.isnan = lambda x: math.isnan(x)
			self.nan = lambda : float('nan')

		else:
			raise ValueError("Unsupported value type: %s" % (self.data_type,))

		self.written_to_base = written_to_base
		self.max_file_item_size = self.value_lenght + self.time_prec + 1

	def value_to_binary(self, value):
		try:
			return struct.pack(self.value_format_string, value)
		except:
			return struct.pack(self.value_format_string, self.nan())

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
		else:
			return time

	def time_just_before(self, time, nanotime):
		if self.time_prec == 8:
			if nanotime == 0:
				return (time - 1, 10 ** 9 - 1)
			else:
				return (time, nanotime - 1)
		else:
			return (time - 1, nanotime)


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
