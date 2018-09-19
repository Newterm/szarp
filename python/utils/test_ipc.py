# -*- coding: utf-8 -*-
"""
TestIPC is a module for testing pythondmn's scripts in SZARP.
"""

# TestIPC is a part of SZARP SCADA software.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.
# get_ipk_path
# autoupdate_sz3
# go_sz4
# get_line_number


__author__    = "Tomasz Pieczerak <tph AT newterm.pl>"
__copyright__ = "Copyright (C) 2016 Newterm"
__version__   = "1.0"
__status__    = "beta"
__email__     = "coders AT newterm.pl"

# imports
import sys
from lxml import etree
from collections import namedtuple
import numpy

def xmlns(f,ns=None):
	def wrapper(element):
		ns2uri = { None : 'http://www.praterm.com.pl/SZARP/ipk',
			'extra':'http://www.praterm.com.pl/SZARP/ipk-extra',
			'exec':'http://www.praterm.com.pl/SZARP/ipk-extra',
			'modbus':'http://www.praterm.com.pl/SZARP/ipk-extra',
			'checker':'http://www.praterm.com.pl/SZARP/ipk-checker',
			'icinga':'http://www.praterm.com.pl/SZARP/ipk-icinga'
			}
		uri = '{'+ns2uri[ns]+'}'+element
		#print str(ns)+':'+element+' ===  '+uri
		return f(uri)
	return wrapper

class TestIPC:
	""" This is a class for testing pythondmn's scripts communication with
	parcook through IPC.
	"""

	ParamInfo = namedtuple('ParamInfo', 'name prec val_type')
	BufInfo = namedtuple('BufInfo', 'index value val_type')

	FunType2Params = {
		'sz3' : 'integer',
		'int' : 'integer',
		'short' : 'integer',
		'long' : 'integer',
		'float' : 'float',
		'double' : 'float'
		}

	#https://www.numpy.org/devdocs/user/basics.types.html
	FunType2numpy = {
		'sz3':numpy.int16,
		'int':numpy.intc,
		'short':numpy.short,
		'long':numpy.int_,
		'float':numpy.single,
		'double':numpy.double
		}

	def __init__(self, prefix, script_path):
		""" Construct IPC object.

		Arguments:
			prefix      - database prefix.
			script_path - full path to your script.
		"""

		self.read_buffer = []
		self.param_list = []
		self.send_list = []
		self.dmn_config = ''

		# read configuration
		filename = "/opt/szarp/%s/config/params.xml" % prefix
		pythondmn_path = '/opt/szarp/bin/pythondmn'
		params_xml = etree.parse(filename, etree.XMLParser(remove_blank_text=True))
		root = params_xml.getroot()

		# search root element for <device> with pythondmn and given script
		dmn_dev = None
		for dev in xmlns(root.findall)('device'):
			if dev.get('daemon') == pythondmn_path and dev.get('path') == script_path:
				dmn_dev = dev	# save element
				break

		if dmn_dev is None:
			sys.stderr.write("ERROR: Cannot find <device> element, bad 'daemon' or 'path' attribute?\n")
			sys.exit(1)

		# fetch parameter list (units are merged)
		for unit in xmlns(dmn_dev.findall)('unit'):
			self.param_list += self.fetch_param_list(unit)
			self.send_list += self.fetch_send_list(unit)
		print "param:",self.param_list
		print "send:",self.send_list

		# remove all elements from <params>
		for i in range(len(root)):
			root.remove(root[0])

		# insert daemon configuration to <params>
		root.append(dmn_dev)

		self.dmn_config = etree.tostring(root, encoding="utf-8", xml_declaration=True)

	def fetch_param_list(self, unit):
		""" Extract parameter list from <unit> element. """
		llist = []
		if unit is None:
			return llist

		for par in xmlns(unit.findall)('param'):
			par_name = par.get('draw_name')
			if par_name == None:
				par_name = par.get('name').split(':')[-1]
			llist.append(TestIPC.ParamInfo(par_name, float(par.get('prec')), xmlns(par.get,'extra')('val_type')))

		return llist


	def fetch_send_list(self, unit):
		""" Extract send list from <unit> element. """
		llist = []
		if unit is None:
			return llist

		for par in xmlns(unit.findall)('send'):
			llist.append(par.get('param'))

		return llist


	def get_conf_str(self):
		""" Return configuration string. """
		return self.dmn_config

	def go_sender(self):
		pass

	def go_parcook(self):
		""" Print all data that were written and clear buffer. """
		for bentry in self.read_buffer:
			i = bentry.index
			v = bentry.value
			try:
				val = float(v) / (10 * self.param_list[i].prec)
			except ValueError:
				val = "NO_DATA"
			except ZeroDivisionError:
				val = v
			print "<-- %s(%s,%s) = %s" % (self.param_list[i].name, bentry.val_type, self.param_list[i].val_type, val)
		self.read_buffer = []

	def sz4_ready(self):
		return True

	def autopublish_sz4(self):
		return None

	def force_sz4(self):
		return None

	def _set_read(self, idx, val, val_type):
		""" Write value to parcook. Used only by this script."""
		params_val_type = self.param_list[idx].val_type

		assert (val_type==None or params_val_type==None or self.FunType2Params[val_type]==params_val_type), "Data type mismatch between params.xml(%s) and function call(%s)" % (params_val_type, val_type)
		assert (val_type=='float' or val_type=='double' or val == type(val)(self.FunType2numpy[val_type](val))), "Conversion of %s from %s to %s failed" % (str(val),str(type(val)),val_type)
		self.read_buffer.append(TestIPC.BufInfo(idx, val, val_type))
		return None

	def set_read(self, idx, val):
		""" Write value to parcook. For sz3 """
		self._set_read(idx, val, 'sz3')

	def set_no_data(self, idx):
		""" Write NO_DATA to parcook. """
		self._set_read(idx, 'NO_DATA', None)

	def set_read_sz4_short(self, idx, val):
		return self._set_read(idx, val, 'short')

	def set_read_sz4_int(self, idx, val):
		return self._set_read(idx, val, 'int')

	def set_read_sz4_long(self, idx, val):
		return self._set_read(idx, val, 'long')

	def set_read_sz4_float(self, idx, val):
		return self._set_read(idx, val, 'float')

	def set_read_sz4_double(self, idx, val):
		return self.set_read_sz4_(idx, val, 'double')


	def _get_send(self,idx, val_type):
		print "--> %s (%s) =" % (self.send_list[idx],val_type),
		val = self.FunType2numpy[val_type](input())
		return val

	def get_send(self,idx):
		return self._get_send(idx,'sz3')

	def get_send_sz4_short(self,idx):
		return self._get_send(idx,'short')

	def get_send_sz4_int(self,idx):
		return self._get_send(idx,'int')

	def get_send_sz4_long(self,idx):
		return self._get_send(idx,'long')

	def get_send_sz4_float(self,idx):
		return self._get_send(idx,'float')

	def get_send_sz4_double(self,idx):
		return self._get_send(idx,'double')


# end of class TestIPC
