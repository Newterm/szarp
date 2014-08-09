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
#include <algorithm>
#include "sz4/defs.h"
#include "sz4/block.h"
#include "sz4/block_cache.h"

namespace sz4 {

generic_block::generic_block(block_cache* cache) :
		m_cache(cache) {
	m_cache->add_new_block(*this);
}

void generic_block::block_data_updated(size_t previous_size) {
	m_cache->block_size_changed(*this, previous_size);
}

void generic_block::remove_from_cache() {
	m_cache->remove_block(*this);
}

generic_block::~generic_block() { 
}

}
