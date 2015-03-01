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

import struct
import StringIO

def encode(delta):
	buf = StringIO.StringIO()

	cnt = bytes_cnt(delta)

	enc_first(buf, cnt, delta)
	add_rest(buf, cnt, delta)

	return buf.getvalue()

def decode(buf):
	cnt, ret = decode_1_byte(byte_from_buf(buf))
	for i in xrange(cnt):
		ret <<= 8
		ret |= byte_from_buf(buf)

	return ret, cnt + 1


def add_rest(buf, cnt, delta):
	for i in xrange(cnt):
		byte = struct.pack("B", (delta >> 8 * (cnt - i - 1)) & 0xff)
		buf.write(byte)

def bytes_after_1(cnt):
	if cnt == 0:
		return 0
	elif cnt >= 7 * 8:
		return (cnt + 7) / 8 
	else:
		return (cnt - 1) / 7

def bytes_cnt(delta):
	bits = number_of_bits(delta)
	return bytes_after_1(bits)


def enc_first(buf, cnt, delta):
	byte = bit_mask(cnt)
	byte |= (delta >> (cnt * 8))

	buf.write(struct.pack("B", byte))
	
def number_of_bits(val):
	assert val >= 0
	bits = 0
	while val:
		bits += 1
		val >>= 1
	return bits

def bit_mask(n):
	return (~(~0 << n) << (8 - n)) & 0xff

def byte_from_buf(buf):
	return struct.unpack("B", buf.read(1))[0]

def decode_1_byte(byte):
	cnt = 0
	mask = 1 << 7
	val_mask = 0x7f

	while byte & mask:
		cnt += 1
		mask >>= 1
		val_mask >>= 1

	return cnt, byte & val_mask

