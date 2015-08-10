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
	def __init__(self, name, data_type, prec, time_prec, written_to_base, combined=False, lsw=None, msw=None):
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
		self.combined = combined
		self.lsw_param_name = lsw
		self.msw_param_name = msw
		self.lsw_combined_referencing_param = None
		self.msw_combined_referencing_param = None
		self.max_file_item_size = self.value_lenght + self.time_prec + 1

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
	def is_combined_formula(formula):
		s = "start"
		for i, c in enumerate(formula):
			if c == '(':
				if s == "start":
					s = "msw_param"
				elif s == "after_msw_param":
					s = "lsw_param"
				else:
					return (False, None, None)
				ps = i + 1
			elif c == ')':
				if s == "msw_param":
					mp = formula[ps:i]
					s = "after_msw_param"
				elif s == "lsw_param":
					lp = formula[ps:i]
					s = "after_lsw_param"
				else:
					return (False, None, None)
			elif c == ':':
				if s == "after_lsw_param":
					s = "done"
				elif s == "msw_param" or s == "lsw_param":
					continue
				else:
					return (False, None, None)
			else:
				if s == "msw_param" or s == "lsw_param" or c.isspace():
					continue
				else:
					return (False, None, None)
			
		if s == "done":
			return (True, mp, lp)
		else:
			return (False, None, None)

	def expand_param_name(string, base_param_name):
		ss = string.split(':')
		ps = base_param_name.split(':')

		for i in range(len(ss)):
			if ss[i] == '*':
				ss[i] = ps[i]

		return ':'.join(ss)

	def prepare_combined(node, param_name):
		try:
			define_node = node.xpath("s:define[@type='DRAWDEFINABLE']", namespaces={'s':'http://www.praterm.com.pl/SZARP/ipk'})[0]
		except IndexError:
			return (False, None, None)

		combined, msw, lsw = is_combined_formula(define_node.attrib['formula'])
		if not combined:
			return (False, None, None)
		else:
			return (True, expand_param_name(lsw, param_name), expand_param_name(msw,param_name))


	param_name = node.attrib["name"]

	combined, lsw, msw = prepare_combined(node, param_name)

	written_to_base = True if "base_ind" in node.attrib or combined else False

	if "data_type" in node.attrib:
		data_type = node.attrib["data_type"]
	elif combined == True:
		data_type = "int"
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

	return Param(param_name, data_type, prec, time_prec, written_to_base, combined, lsw, msw)
