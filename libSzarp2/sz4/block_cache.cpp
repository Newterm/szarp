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

#include "sz4/types.h"
#include "sz4/defs.h"
#include "sz4/block_cache.h"
#include "sz4/block.h"

namespace sz4 {

block_cache::block_cache() : m_cache_size(0) {
	m_blocks.push_back(NULL);
	m_list_separator_position = m_blocks.begin();
}

size_t block_cache::cache_size() const {
	return m_cache_size;
}

void block_cache::add_new_block(generic_block* block) {
	block->location() = m_blocks.insert(m_blocks.end(), block);
}

void block_cache::remove_block(generic_block* block) {
	m_blocks.erase(block->location());
	m_cache_size -= block->block_size();
}

void block_cache::block_size_changed(generic_block* block, size_t previous_size) {
	block_updated(block);

	m_cache_size -= previous_size;
	m_cache_size += block->block_size();
}

void block_cache::block_updated(generic_block* block) {
	m_blocks.erase(block->location());
	m_blocks.insert(block->has_reffering_blocks()
		? m_blocks.end() : m_list_separator_position, block);
}

void block_cache::remove(size_t size) {
	std::list<generic_block*>::iterator i = m_blocks.begin();
	while (i != m_list_separator_position && size > 0) {
		generic_block* block = *i;
		i = m_blocks.begin();
		size -= std::min(size, (*i)->block_size());
		delete block;
	}
}

}
