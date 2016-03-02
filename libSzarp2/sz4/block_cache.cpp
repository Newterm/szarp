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
#include "sz4/block.h"
#include "sz4/block_cache.h"
#include "sz4/defs.h"

namespace sz4 {

block_cache::block_cache() : m_cache_size(0) {
}

void block_cache::cache_size(size_t& size_in_bytes, size_t& blocks_count) const {
	size_in_bytes = m_cache_size;
	blocks_count = m_blocks.size() - 1;
}

void block_cache::add_new_block(generic_block& block) {
	m_blocks.push_back(block);
}

void block_cache::remove_block(generic_block& block) {
	m_blocks.erase(generic_block_list::s_iterator_to(block));
	m_cache_size -= block.block_size();
}

void block_cache::block_size_changed(generic_block& block, size_t previous_size) {
	m_cache_size -= previous_size;
	m_cache_size += block.block_size();
}

void block_cache::block_updated(generic_block& block) {
	m_blocks.erase(generic_block_list::s_iterator_to(block));
	m_blocks.push_back(block);
}

void block_cache::remove(size_t size) {
	while (size > 0 && m_blocks.size()) {
		size -= std::min(size, m_blocks.front().block_size());

		auto block = &m_blocks.front();
		delete block;
	}
}

cache_block_size_updater::cache_block_size_updater(
				generic_block* block)
				:
				m_cache(block->cache()),
				m_block(block) {
	m_previous_size = m_block->block_size();
}

cache_block_size_updater::~cache_block_size_updater() {
	size_t size = m_block->block_size();
	if (size != m_previous_size)
		m_cache->block_size_changed(*m_block, m_previous_size);
}

}
