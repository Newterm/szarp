#!/usr/bin/python
# -*- coding: utf-8 -*-
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


import lxml
import lxml.etree
import os
import tempfile
import unittest
import struct
import shutil

import param
import paramsvalues_pb2
import parampath
import config
import saveparam
import bz2

class SaveParamTest(unittest.TestCase):
	def setUp(self):
		self.node = lxml.etree.fromstring("""
      <param name="Kocioł 3:Sterownik:Aktualne wysterowanie falownika podmuchu" short_name="imp_p" draw_name="Wyst. fal. podm." unit="%" prec="1" base_ind="auto" time_type="nanosecond" data_type="int">
        <raport title="Kocioł 3" order="14"/>
        <draw title="Kocioł 3 Komora spalania" color="red" min="0" max="100" prior="85" order="1"/>
        <draw title="Kocioł 3 Praca falowników" color="red" min="0" max="100" prior="86" order="1"/>
      </param>
""")

	def _msg(self, time, value):
		msg = paramsvalues_pb2.ParamValue()
		msg.param_no = 1
		msg.time = time
		msg.int_value = value

		return msg

	def _unpack(self, path):
		with open(path) as f:
			return bz2.decompress(f.read())

	def _check_size(self, path, size):
		self.assertEqual(len(self._unpack(path)), size)

	def _check_file(self, path, fmt, expected):
		self.assertEqual(struct.unpack(fmt, self._unpack(path)), expected)

	def test_basictest(self):
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		path = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00001234560000000000.sz4")

		sp = saveparam.SaveParam(param.from_node(self.node), temp_dir)
		sp.process_msg_batch([self._msg(123456, 4)])
		self._check_size(path, 4)
		self._check_file(path, "<i", (4, ))

		sp.process_msg_batch([self._msg(123457, 4)])
		self._check_size(path, 9)
		self._check_file(path, "<iBBBBB", (4, 0xf0, 0x3b, 0x9a, 0xca, 0x00))

		sp.process_msg_batch([self._msg(123458, 5)])
		self._check_size(path, 4 + 5 + 4)
		self._check_file(path, "<iBBBBBi", (4, 0xf0, 0x77, 0x35, 0x94, 0x00, 5))

		del sp

		shutil.rmtree(temp_dir)

	def test_basictest2(self):
		"""Similar to basictest, but creates new saveparam object before writing second value"""
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		path = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00001234560000000000.sz4")

		sp = saveparam.SaveParam(param.from_node(self.node), temp_dir)
		sp.process_msg_batch([self._msg(123456, 4)])
		self._check_size(path, 4)
		self._check_file(path, "<i", (4, ))

		sp.process_msg_batch([self._msg(123457, 5)])
		self._check_size(path, 13)
		self._check_file(path, "<iBBBBBi", (4, 0xf0, 0x3b, 0x9a, 0xca, 0x00, 5))

		del sp
		sp = saveparam.SaveParam(param.from_node(self.node), temp_dir)

		sp.process_msg_batch([ self._msg(123458, 5) ])
		self._check_size(path, 27)
		self._check_file(path, "<iBBBBBiBiBBBBBi",
					(4, 0xf0, 0x3b, 0x9a, 0xca, 0x00,
					5, 0x01,
					-2**31, 0xf0, 0x3b, 0x9a, 0xc9, 0xff,
					5, ))

		shutil.rmtree(temp_dir)

	def test_newfiletest(self):
		"""Test if the proper switching to new file occurs"""
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")

		i = 0
		prev_size = 0	
		sp = saveparam.SaveParam(param.from_node(self.node), temp_dir)
		sp.data_file_size = 100

		while True:
			sp.process_msg_batch([self._msg(i, i)])

			size = len(sp.file.getvalue())
			if prev_size > size:
				break

			prev_size = size
			i += 1

		sp.commit()
			
		path2 = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/%010d0000000000.sz4" % (i-1))
		self._check_file(path2, "<iBBBBBi", (i - 1, 0xf0, 0x3b, 0x9a, 0xca, 0x00, i))

		shutil.rmtree(temp_dir)
