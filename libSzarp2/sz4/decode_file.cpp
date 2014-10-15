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
#include "sz4/real_param_entry.h"

namespace sz4 {

void decode_first_byte(unsigned char c, size_t& count, long long& value) {
	count = 0;
	value = 0;

	int mask = 128;
	int value_mask = 0x7f;

	while (mask & c) {
		count += 1;
		mask >>= 1;
		value_mask >>= 1;
	}

	value = c & value_mask;
}

void decode_remaining_bytes(const unsigned char* buffer, size_t size, const size_t& count, long long& delta) {
	for (size_t i = 0; i < count; i++) {
		delta <<= 8;
		delta |= buffer[i + 1];
	}
}

size_t decode_delta(long long& delta, const unsigned char* buffer, size_t size) {
	size_t count;

	decode_first_byte(*buffer, count, delta);
	decode_remaining_bytes(buffer, size, count, delta);
	
	return count + 1;
}

}
