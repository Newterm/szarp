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


__author__    = "Tomasz Pieczerak <tph AT newterm.pl>"
__copyright__ = "Copyright (C) 2016 Newterm"
__version__   = "1.0"
__status__    = "beta"
__email__     = "coders AT newterm.pl"

# imports
from lxml import etree
from collections import namedtuple

class TestIPC:
	""" This is a class for testing pythondmn's scripts communication with
	parcook through IPC.
	"""

	ParamInfo = namedtuple('ParamInfo', 'name prec')
	BufInfo = namedtuple('BufInfo', 'index value')

	def __init__(self, prefix, script_path):
		""" Construct IPC object.

		Arguments:
			prefix      - database prefix.
			script_path - full path to your script.
		"""
		self.read_buffer = []
		self.param_list = []
		self.dmn_config = ''

		# read configuration
		filename = "/opt/szarp/%s/config/params.xml" % prefix
		pythondmn_path = '/opt/szarp/bin/pythondmn'
		params_xml = etree.parse(filename, etree.XMLParser(remove_blank_text=True))
		root = params_xml.getroot()

		# search root element for <device> with pythondmn and given script
		for dev in root.findall('./{http://www.praterm.com.pl/SZARP/ipk}device'):
			if dev.get('daemon') == pythondmn_path and dev.get('path') == script_path:
				dmn_dev = dev	# save element
				break

		# fetch parameter list (units are merged)
		for unit in dmn_dev.findall('./{http://www.praterm.com.pl/SZARP/ipk}unit'):
			self.param_list += self.fetch_param_list(unit)

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

		for par in unit.findall('./{http://www.praterm.com.pl/SZARP/ipk}param'):
			llist.append(TestIPC.ParamInfo(par.get('draw_name'), float(par.get('prec'))) )

		return llist

	def get_conf_str(self):
		""" Return configuration string. """
		return self.dmn_config

	def set_read(self, idx, val):
		""" Write value to parcook. """
		self.read_buffer.append(TestIPC.BufInfo(idx, val))

	def set_no_data(self, idx):
		""" Write NO_DATA to parcook. """
		self.read_buffer.append(TestIPC.BufInfo(idx, 'NO_DATA'))

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
			print "%s = %s" % (self.param_list[i].name, val)
		self.read_buffer = []

# end of class TestIPC
