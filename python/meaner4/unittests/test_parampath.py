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
import param
import parampath
import unittest
import os
import tempfile
import shutil

class ParamPathTest(unittest.TestCase):

	def setUp(self):
		node1 = lxml.etree.fromstring("""
      <param name="Kocioł 3:Sterownik:Aktualne wysterowanie falownika podmuchu" short_name="imp_p" draw_name="Wyst. fal. podm." unit="%" prec="1" base_ind="auto">
        <raport title="Kocioł 3" order="14"/>
        <draw title="Kocioł 3 Komora spalania" color="red" min="0" max="100" prior="85" order="1"/>
        <draw title="Kocioł 3 Praca falowników" color="red" min="0" max="100" prior="86" order="1"/>
      </param>
""")
		self.param1 = param.Param(node1)

		node2 = lxml.etree.fromstring("""
      <param name="Kocioł 3:Sterownik:Aktualne wysterowanie falownika podmuchu" short_name="imp_p" draw_name="Wyst. fal. podm." unit="%" prec="1" base_ind="auto" time_type="nanosecond">
        <raport title="Kocioł 3" order="14"/>
        <draw title="Kocioł 3 Komora spalania" color="red" min="0" max="100" prior="85" order="1"/>
        <draw title="Kocioł 3 Praca falowników" color="red" min="0" max="100" prior="86" order="1"/>
      </param>
""")
		self.param2 = param.Param(node2)

	def test_path1(self):
		pp = parampath.ParamPath(self.param1, "/tmp")
		self.assertEqual(pp.create_file_path(1, 1),
			"/tmp/Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/0000000001.sz4")

	def test_path2(self):
		pp = parampath.ParamPath(self.param2, "/tmp")
		self.assertEqual(pp.create_file_path(1, 1),
			"/tmp/Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00000000010000000001.sz4")


	def test_path3(self):
		temp_dir = tempfile.mkdtemp(suffix="meaner4_unit_test")

		pp = parampath.ParamPath(self.param2, temp_dir)
		os.makedirs(os.path.dirname(pp.create_file_path(0, 0)))
	
		for i in range(0, 10):
			with open(pp.create_file_path(i, i), "w") as f:
				pass

		self.assertEqual(pp.find_latest_path(),
			temp_dir + "/Kociol_3/Sterownik/Aktualne_wysterowanie_falownika_podmuchu/00000000090000000009.sz4")
		shutil.rmtree(temp_dir)
