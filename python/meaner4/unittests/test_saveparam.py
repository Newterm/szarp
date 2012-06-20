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

class SaveParamTest(unittest.TestCase):
	def setUp(self):
		self.node = lxml.etree.fromstring("""
      <param name="Kocioł 3:Sterownik:Aktualne wysterowanie falownika podmuchu" short_name="imp_p" draw_name="Wyst. fal. podm." unit="%" prec="1" base_ind="auto" time_prec="8" data_type="int">
        <raport title="Kocioł 3" order="14"/>
        <draw title="Kocioł 3 Komora spalania" color="red" min="0" max="100" prior="85" order="1"/>
        <draw title="Kocioł 3 Praca falowników" color="red" min="0" max="100" prior="86" order="1"/>
      </param>
""")

	def test_saveparam(self):
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")
		sp = saveparam.SaveParam(self.node, temp_dir)

		msg = paramsvalues_pb2.ParamValue()
		msg.param_no = 1;
		msg.time = 123456;
		msg.int_value = 4
		sp.process_msg(msg)

		file_path = os.path.join(temp_dir, "Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00001234560000000000.sz4")
		st = os.stat(file_path)
		self.assertEqual(st.st_mtime, 123456.)

		with open(file_path) as f:
			self.assertEqual(struct.unpack("<i", f.read(4))[0], 4)

		shutil.rmtree(temp_dir)
