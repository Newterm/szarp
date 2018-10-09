# -*- coding: utf-8 -*-
"""
Contains function add_xmlns that adds prefix url to given namespace
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


__author__    = "Adam Brodacki <abrodacki AT newterm.pl>"
__copyright__ = "Copyright (C) 2018 Newterm"


def add_xmlns(f,ns=None):
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

