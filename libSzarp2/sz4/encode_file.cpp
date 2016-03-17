/* 
  SZARP: SCADA software 
  

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
*/

#include "sz4/defs.h"
#include "sz4/block.h"
#include "sz4/block_cache.h"
#include "sz4/base.h"
#include "sz4/buffer.h"
#include "sz4/util.h"
#include "sz4/encode_file.h"

namespace sz4 {

namespace {

int delta_size_bits(unsigned long long delta) {
	int ret = 0;
	for (; delta; delta >>= 1)
		ret += 1;
	return ret;
}

int delta_size_bytes(int delta_bits) {
	if (delta_bits == 0)
		return 1;
	else if (delta_bits >= 7 * 8)
		return (delta_bits + 7) / 8 + 1;
	else
		return (delta_bits - 1) / 7 + 1;
}

unsigned char delta_first_byte_mask(int delta_size) {
	return static_cast<unsigned char>(~0u << (8 - 1));
}

}

size_t encode_delta(const long long& delta, unsigned char output[8]) {
	int delta_bits = delta_size_bits(delta);
	int delta_bytes = delta_size_bytes(delta_bits);
	unsigned char mask = delta_first_byte_mask(delta_bits);

	output[0] = mask | (delta >> (delta_bytes * 8));
	for (int i = 1; i < delta_bytes; i++)
		output[i] = delta >> (delta_bytes - i);

	return delta_bytes;
}

}

