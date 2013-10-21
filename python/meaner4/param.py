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

		self.prepare_combined(node)

		self.written_to_base = True if "base_ind" in node.attrib or self.combined else False

		if "data_type" in node.attrib:
			self.data_type = node.attrib["data_type"]
		elif self.combined:
			self.data_type = "int"	
		else:
			self.data_type = "short"

		if self.combined or "prec" not in node.attrib:
			self.prec = 0
		else:
			self.prec = int(node.attrib["prec"])

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

		self.lsw_combined_referencing_param = None
		self.msw_combined_referencing_param = None

	def expand_param_name(self, string, base_param_name):
		ss = string.split(':')
		ps = base_param_name.split(':')

		for i in range(len(ss)):
			if ss[i] == '*':
				ss[i] = ps[i]

		return ':'.join(ss)
	

	def is_combined_formula(self, formula):
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


	def prepare_combined(self, node):
		try:
			define_node = node.xpath("s:define[@type='DRAWDEFINABLE']", namespaces={'s':'http://www.praterm.com.pl/SZARP/ipk'})[0]
		except IndexError:
			self.combined = False
			return

		self.combined, msw, lsw = self.is_combined_formula(define_node.attrib['formula'])
		if not self.combined:
			return

		self.lsw_param_name = self.expand_param_name(lsw, self.param_name)
		self.msw_param_name = self.expand_param_name(msw, self.param_name)
		self.combined = True
		
	def nan(self):
		if self.data_type == "short":
			return -2**15
		elif self.data_type == "int":
			return -2**31
		else:
			return float('nan')

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

	def time_just_before(self, time, nanotime):
		if self.time_prec == 8:
			if nanotime == 0:
				return (time - 1, 10 ** 9 - 1)
			else:
				return (time, nanotime - 1)
		else:
			return (time - 1, nanotime)


