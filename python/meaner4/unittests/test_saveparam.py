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
from itertools import chain

import param
import paramsvalues_pb2
import parampath
import config
import saveparam
import bz2

SEC_NS = (0xf0, 0x3b, 0x9a, 0xca, 0x00)
_999999999_NS = (0xf0, 0x3b, 0x9a, 0xc9, 0xff)

class SaveParamTest(unittest.TestCase):
	def setUp(self):
		self.node = lxml.etree.fromstring("""
      <param name="Kocioł 3:Sterownik:Aktualne wysterowanie falownika podmuchu" short_name="imp_p" draw_name="Wyst. fal. podm." unit="%" prec="1" base_ind="auto" time_type="nanosecond" data_type="uinteger">
        <raport title="Kocioł 3" order="14"/>
        <draw title="Kocioł 3 Komora spalania" color="red" min="0" max="100" prior="85" order="1"/>
        <draw title="Kocioł 3 Praca falowników" color="red" min="0" max="100" prior="86" order="1"/>
      </param>
""")
		self.param = param.from_node(self.node)

		self.temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		self.param_dir = os.path.join(self.temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu")

	def tearDown(self):
		shutil.rmtree(self.temp_dir)


	def _msg(self, value, time, nanotime = 0):
		msg = paramsvalues_pb2.ParamValue()
		msg.param_no = 1
		msg.time = time
		msg.nanotime = nanotime
		msg.int_value = value

		return msg

	def _file_size(self, f):
		p = f.tell()

		f.seek(0, 2)
		s = f.tell()

		f.seek(p, 0)
		return s

	def _read(self, path):
		with open(path) as f:
			return f.read()

	def _unpack(self, path):
		with open(path) as f:
			return bz2.decompress(f.read())

	def _check_size_bz(self, path, size):
		self.assertEqual(len(self._unpack(path)), size)

	def _check_file_bz(self, path, fmt, expected):
		print [ord(x) for x in self._unpack(path)]
		print len([ord(x) for x in self._unpack(path)])
		print expected
		self.assertEqual(struct.unpack(fmt, self._unpack(path)), expected)

	def _check_size(self, path, size):
		self.assertEqual(len(self._read(path)), size)

	def _check_file(self, path, fmt, expected):
		print [ord(x) for x in self._read(path)]
		print len([ord(x) for x in self._read(path)])
		print expected
		self.assertEqual(struct.unpack(fmt, self._read(path)), expected)

	def basictest(self):
		path = os.path.join(self.param_dir, "42949672954294967294.sz4")

		sp = saveparam.SaveParam(self.param, self.temp_dir)
		sp.process_msgs([self._msg(4, 123456)])
		self._check_size(path, 12)
		self._check_file(path, "<IIi", (123456, 0, 4))

		sp.process_msgs([self._msg(4, 123457)])
		self._check_size(path, 21)
		self._check_file(path, "<IIiBBBBBi", (123456, 0, 4 ) + SEC_NS + (4,))

		sp.process_msgs([self._msg(5, 123458)])
		self._check_size(path, 8 + 4 + 5 + 4 + 5 + 4)
		self._check_file(path, "<IIiBBBBBiBBBBBi", (123456, 0, 4) + SEC_NS +  (4,) + SEC_NS + (5,))

		return sp, path

	def test_basictest(self):
		sp, _ = self.basictest()
		del sp

	def test_basictest2(self):
		"""Similar to basictest, but creates new saveparam object before writing second value"""
		sp, path = self.basictest()
		sp.close()
		del sp

		sp = saveparam.SaveParam(self.param, self.temp_dir)

		sp.process_msgs([ self._msg(5, 123459) ])
		self._check_size(path, 12 + 5 + 4 + 5 + 4 + 1 + 4 + 5 + 4)
		self._check_file(path, "<IIiBBBBBiBBBBBiBiBBBBBi", 
					(123456, 0, 4) + \
					 SEC_NS + (4,) + \
					 SEC_NS + (5,) + \
					 (0x01, self.param.nan()) + \
					 _999999999_NS + (5,))

	def newfiletest_start(self):

		i = 0
		prev_size = 0	
		sp = saveparam.SaveParam(self.param, self.temp_dir)
		sp.compressed_file_size = 100
		sp.uncompressed_file_size = 100

		while True:
			sp.process_msgs([self._msg(i, i)])

			size = self._file_size(sp.file)
			if prev_size > size:
				break

			prev_size = size
			i += 1

		path = os.path.join(self.param_dir, "42949672954294967294.sz4")
                self._check_file(path, "<IIiBBBBBi", (i - 1, 1, i - 1) + _999999999_NS + (i,))

		path2 = os.path.join(self.param_dir, "00000000000000000000.sz4")
		expected_bz = chain.from_iterable([ (x,) + SEC_NS for x in range(i - 1) ] + [[ i - 1 ]])
		self._check_file_bz(path2, "<" + "iBBBBB" * (i - 1) + "i", tuple(expected_bz))

		return i, sp

	def test_newfiletest(self):
		"""Test if the proper switching to new file occurs"""
		self.newfiletest_start()

	def test_startfrombz(self):
		"""Test if the proper start of saveparam happens with bz2 file only"""
		i, _ = self.newfiletest_start()
		os.remove(os.path.join(self.param_dir, "42949672954294967294.sz4"))

		sp = saveparam.SaveParam(self.param, self.temp_dir)
		sp.process_msgs([self._msg(i, i)])

		path = os.path.join(self.param_dir, "42949672954294967294.sz4")
		self._check_file(path, "<IIiBBBBBi", (i - 1, 1) + (self.param.nan(),) + _999999999_NS + (i,))

	def test_startfrombznogap(self):
		"""Test if the proper start of saveparam happens with bz2 file only"""
		i, _ = self.newfiletest_start()
		os.remove(os.path.join(self.param_dir, "42949672954294967294.sz4"))

		sp = saveparam.SaveParam(self.param, self.temp_dir)
		sp.process_msgs([self._msg(i, i - 1, 1)])

		path = os.path.join(self.param_dir, "42949672954294967294.sz4")
		self._check_file(path, "<IIi", (i - 1, 1, i))

	def test_appendingtobzip(self):
		"""Test if the proper start of saveparam happens with bz2 file only"""

		i, sp = self.newfiletest_start()

		prev_size = self._file_size(sp.file)

		while True:
			i += 1

			sp.process_msgs([self._msg(i, i)])

			size = self._file_size(sp.file)
			if prev_size > size:
				break

			prev_size = size

		path = os.path.join(self.param_dir, "00000000000000000000.sz4")
		expected_bz = chain.from_iterable([ (x,) + SEC_NS for x in range(i - 1) ] + [[ i - 1 ]])
		self._check_file_bz(path, "<" + "iBBBBB" * (i - 1) + "i", tuple(expected_bz))

		path = os.path.join(self.param_dir, "42949672954294967294.sz4")
		self._check_file(path, "<IIiBBBBBi", (i - 1, 1, i - 1) + _999999999_NS + (i,))

	def test_bzipfileoverflow(self):
		i, sp = self.newfiletest_start()

		prev_size = self._file_size(sp.file)
		prev_bz2_size = os.stat(os.path.join(self.param_dir, "00000000000000000000.sz4")).st_size

		while True:
			i += 1

			sp.process_msgs([self._msg(i, i)])

			size = self._file_size(sp.file)
			if prev_size > size:
				bz2_size = os.stat(os.path.join(self.param_dir, "00000000000000000000.sz4")).st_size
				if bz2_size == prev_bz2_size:
					break
				else:
					prev_bz2_size = bz2_size
					last_bz2_i = i

			prev_size = size

		path = os.path.join(self.param_dir, "00000000000000000000.sz4")
		expected_bz = chain.from_iterable([ (x,) + SEC_NS for x in range(last_bz2_i  - 1) ] + [[ last_bz2_i - 1 ]])
		self._check_file_bz(path, "<" + "iBBBBB" * (last_bz2_i - 1) + "i", tuple(expected_bz))
