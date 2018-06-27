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
import unittest

import param
import paramsvalues_pb2

class ParamTest(unittest.TestCase):
	def setUp(self):

		self.types 	= [ "short"        , "integer"       , "float"       , "double"       , "ushort"    , "uinteger"    ]
		self.val_lens   = [ 2              , 4           , 4             , 8              , 2           , 4           ]
		self.formats	= [ "<h"           , "<i"        , "<f"          , "<d"           , "<H"       , "<I"       ]
		self.val_cast = [lambda x: int(x), lambda x: int(x), lambda x: x, lambda x: x, lambda x: int(x), lambda x: int(x)]
		self.values	= [ 1	           , 1000000     , 1.5           , 1.234567889    , 40000       , 3000000  ]
		self.epsilons = [0, 0, 10**-7, 10**-10, 0, 0]
		self.msg_attrs  = [ "int_value"	   , "int_value" , "double_value" , "double_value" , "int_value" , "int_value" ]

		self.node_def = lxml.etree.fromstring("""
      <param xmlns="http://www.praterm.com.pl/SZARP/ipk" name="Kocioł 3:Sterownik:Aktualne wysterowanie falownika podmuchu" short_name="imp_p" draw_name="Wyst. fal. podm." unit="%" prec="1" base_ind="auto">
        <raport title="Kocioł 3" order="14"/>
        <draw title="Kocioł 3 Komora spalania" color="red" min="0" max="100" prior="85" order="1"/>
        <draw title="Kocioł 3 Praca falowników" color="red" min="0" max="100" prior="86" order="1"/>
      </param>
""")

		self.nodes = []
		for t in self.types:
			n = lxml.etree.fromstring("""
      <param xmlns="http://www.praterm.com.pl/SZARP/ipk" name="Kocioł 3:Sterownik:Aktualne wysterowanie falownika podmuchu" short_name="imp_p" draw_name="Wyst. fal. podm." unit="%%" prec="1" base_ind="auto" data_type="%s">
        <raport title="Kocioł 3" order="14"/>
        <draw title="Kocioł 3 Komora spalania" color="red" min="0" max="100" prior="85" order="1"/>
        <draw title="Kocioł 3 Praca falowników" color="red" min="0" max="100" prior="86" order="1"/>
      </param>
""" % (t))
			self.nodes.append(n)

	def test_parsing_def(self):
		p = param.from_node(self.node_def)

		self.assertEqual(p.data_type, "short")
		self.assertEqual(p.value_format_string, "<h")
		self.assertEqual(p.value_lenght, 2)
		self.assertEqual(p.time_prec, 4)
		self.assertEqual(p.written_to_base, True)
		self.assertEqual(p.value_from_binary(p.value_to_binary(10)), 10)

		msg = paramsvalues_pb2.ParamValue()
		msg.param_no = 100;
		msg.is_nan = False;
		msg.time = 123456;
		msg.int_value = 10;

		self.assertEqual(p.value_from_msg(msg), 10)

	def test_parsing2(self):
		for i, n in enumerate(self.nodes):
			p = param.from_node(n)

			self.assertEqual(p.data_type, self.types[i])
			self.assertEqual(p.value_format_string, self.formats[i])
			self.assertEqual(p.value_lenght, self.val_lens[i])
			self.assertEqual(p.time_prec, 4)
			self.assertEqual(p.written_to_base, True)
			self.assertEqual(p.value_from_binary(p.value_to_binary(self.values[i])), self.values[i])

			msg = paramsvalues_pb2.ParamValue()
			msg.param_no = 100;
			msg.is_nan = False;
			msg.time = 123456;
			setattr(msg, self.msg_attrs[i], self.values[i])

			self.assertEqual(p.value_from_msg(msg), self.values[i])

	def test_crosscast_types(self):
		from random import randint
		int_value = randint(1, 2000)
		float_value = randint(1, 2000) / 10.0 # prec 1
		for i, n in enumerate(self.nodes):
			p = param.from_node(n)

			self.assertEqual(p.data_type, self.types[i])
			self.assertEqual(p.value_format_string, self.formats[i])
			self.assertEqual(p.value_lenght, self.val_lens[i])
			self.assertEqual(p.time_prec, 4)
			self.assertEqual(p.written_to_base, True)
			self.assertTrue(p.value_from_binary(p.value_to_binary(int_value)) - self.val_cast[i](int_value) <= int_value*self.epsilons[i])
			self.assertTrue(p.value_from_binary(p.value_to_binary(float_value)) - self.val_cast[i](float_value) <= int_value*self.epsilons[i])

			msg = paramsvalues_pb2.ParamValue()
			msg.param_no = 100;
			msg.is_nan = False;
			msg.time = 123456;
			setattr(msg, self.msg_attrs[i], self.values[i])

			self.assertEqual(p.value_from_msg(msg), self.values[i])

	def test_casting_nans(self):
		def _msg(is_nan):
			msg = paramsvalues_pb2.ParamValue()
			msg.param_no = 100;
			msg.time = 123456;
			msg.is_nan = is_nan
			return msg

		from random import randint
		for i, n in enumerate(self.nodes):
			p = param.from_node(n)
			self.assertTrue(p.isnan(p.nan))

			# nan msg w/o value
			msg = _msg(True)
			self.assertTrue(p.isnan(p.value_from_msg(msg)))

			# nan w/ value
			msg.double_value = 10.4
			msg.int_value = 104
			self.assertTrue(p.isnan(p.value_from_msg(msg)))

			# non-nan w/ value
			msg = _msg(False)
			msg.int_value = 104
			self.assertFalse(p.isnan(p.value_from_msg(msg)))

			msg = _msg(False)
			msg.double_value = 10.4
			self.assertFalse(p.isnan(p.value_from_msg(msg)))

			# non-nan w/o value (should be nan)
			msg = _msg(False)
			self.assertTrue(p.isnan(p.value_from_msg(msg)))

			# non-nan w/ nan value (should be nan)
			msg = _msg(False)
			setattr(msg, self.msg_attrs[i], p.nan)
			self.assertTrue(p.isnan(p.value_from_msg(msg)))

			msg = _msg(False)
			msg.double_value = float('nan')
			self.assertTrue(p.isnan(p.value_from_msg(msg)))
