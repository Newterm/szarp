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
      <param name="Kocioł 3:Sterownik:Aktualne wysterowanie falownika podmuchu" short_name="imp_p" draw_name="Wyst. fal. podm." unit="%" prec="1" base_ind="auto" time_type="nanosecond" data_type="integer">
        <raport title="Kocioł 3" order="14"/>
        <draw title="Kocioł 3 Komora spalania" color="red" min="0" max="100" prior="85" order="1"/>
        <draw title="Kocioł 3 Praca falowników" color="red" min="0" max="100" prior="86" order="1"/>
      </param>
""")
		self.float_node = lxml.etree.fromstring("""
      <param name="A:B:float" short_name="imp_p" unit="%" prec="1" base_ind="auto" time_type="second" data_type="float"/>""")

	def _msg(self, time, value):
		msg = paramsvalues_pb2.ParamValue()
		msg.param_no = 1
		msg.time = time
		msg.is_nan = False
		msg.int_value = value

		return msg

	def _check_doesnt_exist(self, path):
		self.assertFalse(os.path.isfile(path))

	def _check_size(self, path, size):
		st = os.stat(path)
		self.assertEqual(st.st_size, size)

	def _check_file(self, path, fmt, expected):
		with open(path) as f:
			self.assertEqual(struct.unpack(fmt, f.read()), expected)

	def test_basictest(self):
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		path = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00001234560000000000.sz4")

		sp = saveparam.SaveParam(param.from_node(self.node), temp_dir)
		sp.process_msg(self._msg(123456, 4))
		self._check_size(path, 4)
		self._check_file(path, "<i", (4, ))

		sp.process_msg(self._msg(123457, 4))
		self._check_size(path, 9)
		self._check_file(path, "<iBBBBB", (4, 0xf0, 0x3b, 0x9a, 0xca, 0x00))

		sp.process_msg(self._msg(123458, 5))
		self._check_size(path, 4 + 5 + 4)
		self._check_file(path, "<iBBBBBi", (4, 0xf0, 0x77, 0x35, 0x94, 0x00, 5))

		del sp

		shutil.rmtree(temp_dir)

	def test_basictest2(self):
		"""Similar to basictest, but creates new saveparam object before writing second value"""
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		path = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00001234560000000000.sz4")

		sp = saveparam.SaveParam(param.from_node(self.node), temp_dir)
		sp.process_msg(self._msg(123456, 4))
		self._check_size(path, 4)
		self._check_file(path, "<i", (4, ))

		sp.process_msg(self._msg(123457, 4))
		self._check_size(path, 9)
		self._check_file(path, "<iBBBBB", (4, 0xf0, 0x3b, 0x9a, 0xca, 0x00))

		# check if will properly fill nans
		del sp
		sp = saveparam.SaveParam(param.from_node(self.node), temp_dir)

		sp.process_msg(self._msg(123458, 5))
		self._check_size(path, 22)
		self._check_file(path, "<iBBBBBiBBBBBi",
					(4, 0xf0, 0x3b, 0x9a, 0xca, 0x00,
					-2**31, 0xf0, 0x3b, 0x9a, 0xca, 0x00,
					5, ))

		shutil.rmtree(temp_dir)

	def test_basictest3(self):
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")

		item_size = 4 + 5
		items_per_file = config.DATA_FILE_SIZE / item_size

		sp = saveparam.SaveParam(param.from_node(self.node), temp_dir)
		for i in range(items_per_file):
			sp.process_msg(self._msg(i, i))

		path = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00000000000000000000.sz4")
		self._check_size(path, (items_per_file - 1) * item_size)

		path2 = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/%010d0000000000.sz4" % (items_per_file - 1))
		self._check_size(path2, 4)

		shutil.rmtree(temp_dir)

	def test_processing_float_nans(self):
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		path = os.path.join(temp_dir, "A/B/float/0000123457.sz4")
		spf = saveparam.SaveParam(param.from_node(self.float_node), temp_dir)

		for t,v in [(123456, spf.param.nan), (123457, 4.5), (123458, spf.param.nan), (123459, spf.param.nan), (123460, 5.0), (123461, 5.0)]:
			msg = paramsvalues_pb2.ParamValue()
			msg.param_no = 1
			msg.time = t
			msg.is_nan = False
			msg.double_value = v
			spf.process_msg(msg)

		self._check_size(path, 15)
		self._check_file(path, "<fBBBBBBfB", (4.5, 1, 0x00, 0x00, 0xc0, 0x7f, 2, 5.0, 1))

		shutil.rmtree(temp_dir)

	def test_processing_batch(self):
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		path = os.path.join(temp_dir, "A/B/float/0000123457.sz4")
		spf = saveparam.SaveParam(param.from_node(self.float_node), temp_dir)

		msgs = []
		for t,v in [(123456, spf.param.nan), (123457, 4.5), (123458, spf.param.nan), (123459, spf.param.nan), (123460, 5.0), (123461, 5.0)]:
			msg = paramsvalues_pb2.ParamValue()
			msg.param_no = 1
			msg.time = t
			msg.is_nan = False
			msg.double_value = v
			msgs.append(msg)

		s_time = spf.process_msg_batch(msgs)
		self.assertEqual(s_time, 123461)

		self._check_size(path, 15)
		self._check_file(path, "<fBBBBBBfB", (4.5, 1, 0x00, 0x00, 0xc0, 0x7f, 2, 5.0, 1, ))

		shutil.rmtree(temp_dir)

	def test_nan_handling(self):
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		def _path(time):
			return os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/0000"+str(time)+"0000000000.sz4")
		sp = saveparam.SaveParam(param.from_node(self.node), temp_dir)

		# check if will properly handle nan msg with nan value
		nan_v = sp.param.nan
		msg = self._msg(123456, nan_v)
		msg.is_nan = True
		sp.process_msg(msg)
		self._check_doesnt_exist(_path(123456))

		# check if will properly handle nan msg with incorrect value
		msg = self._msg(123457, 104)
		msg.is_nan = True
		sp.process_msg(msg)
		self._check_doesnt_exist(_path(123457))

		# check if will properly handle non-nan msg with nan value
		msg = self._msg(123458, nan_v)
		sp.process_msg(msg)
		self._check_doesnt_exist(_path(123458))

		# check if will properly handle regular value after nans
		path = _path(123459)
		sp.process_msg(self._msg(123459, 4))
		self._check_size(path, 4)
		self._check_file(path, "<i", (4, ))

		# check if will properly handle regular value after nan
		msg = self._msg(123460, 0)
		msg.is_nan = True
		sp.process_msg(msg)

		self._check_size(path, 13)
		self._check_file(path, "<iBBBBBi",
					(4, 0xf0, 0x3b, 0x9a, 0xca, 0x00,
					-2**31, ))

		sp.process_msg(self._msg(123461, 8))

		self._check_size(path, 22)
		self._check_file(path, "<iBBBBBiBBBBBi",
					(4, 0xf0, 0x3b, 0x9a, 0xca, 0x00,
					-2**31, 0xf0, 0x3b, 0x9a, 0xca, 0x00,
					8, ))

		shutil.rmtree(temp_dir)
