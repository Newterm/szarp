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

import unittest
import timedelta
import StringIO

class EncodeDecodeTest(unittest.TestCase):
	def test_decode_encode_def(self):
		#test encoding of 0
		encoded = timedelta.encode(0)
		self.assertEqual(chr(0x0), encoded[0])

		#some arbitrary long enough number
		v = 33958800
		encoded = timedelta.encode(v)
		self.assertEqual(4, len(encoded))

		self.assertEqual(chr(0xe2), encoded[0])
		self.assertEqual(chr(0x06), encoded[1])
		self.assertEqual(chr(0x2b), encoded[2])
		self.assertEqual(chr(0x90), encoded[3])

		year_in_ns = 365 * 24 * 3600 * 10 ** 9
		for v in range(0, 10 * year_in_ns, year_in_ns / 50):
			encoded = timedelta.encode(v)
			decoded, _ = timedelta.decode(StringIO.StringIO(encoded))

			self.assertEqual(v, decoded)
