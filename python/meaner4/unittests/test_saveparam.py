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
import saveparam
import config

class SaveParamTest(unittest.TestCase):
	def setUp(self):
		self.node = lxml.etree.fromstring("""
      <param name="Kocioł 3:Sterownik:Aktualne wysterowanie falownika podmuchu" short_name="imp_p" draw_name="Wyst. fal. podm." unit="%" prec="1" base_ind="auto" time_prec="8" data_type="int">
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

	def _check_size(self, path, size):
		st = os.stat(path)
		self.assertEqual(st.st_size, size)

	def _check_file(self, path, fmt, expected):
		with open(path) as f:
			self.assertEqual(struct.unpack(fmt, f.read()), expected)

	def test_basictest(self):
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		path = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00001234560000000000.sz4")

		sp = saveparam.SaveParam(param.Param(self.node), temp_dir)
		sp.process_msg(self._msg(123456, 4))
		self._check_size(path, 12)
		self._check_file(path, "<iII", (4, 123456, 0))

		sp.process_msg(self._msg(123457, 4))
		self._check_size(path, 12)
		self._check_file(path, "<iII", (4, 123457, 0))

		sp.process_msg(self._msg(123458, 5))
		self._check_size(path, 4 + 8 + 4 + 8)
		self._check_file(path, "<iIIiII", (4, 123457, 0, 5, 123458, 0))

		del sp

		shutil.rmtree(temp_dir)

	def test_basictest2(self):
		"""Similar to basictest, but creates new saveparam object before writing second value"""
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		path = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00001234560000000000.sz4")

		sp = saveparam.SaveParam(param.Param(self.node), temp_dir)
		sp.process_msg(self._msg(123456, 4))
		self._check_size(path, 12)
		self._check_file(path, "<iII", (4, 123456, 0))

		sp.process_msg(self._msg(123457, 4))
		self._check_size(path, 12)
		self._check_file(path, "<iII", (4, 123457, 0))

		del sp
		sp = saveparam.SaveParam(param.Param(self.node), temp_dir)

		sp.process_msg(self._msg(123459, 5))
		self._check_size(path, 4 + 8 + 4 + 8 + 4 + 8)
		self._check_file(path, "<iIIiIIiII", (4, 123457, 0, -2**31, 123458, 999999999, 5, 123459, 0))

		shutil.rmtree(temp_dir)

	def test_basictest3(self):
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")

		item_size = 4 + 8
		items_per_file = config.DATA_FILE_SIZE / item_size

		sp = saveparam.SaveParam(param.Param(self.node), temp_dir)
		for i in range(items_per_file + 1):
			sp.process_msg(self._msg(i, i))

		path = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00000000000000000000.sz4")
		self._check_size(path, items_per_file * item_size)

		path2 = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/%010d0000000000.sz4" % (items_per_file))
		self._check_size(path2, 12)
