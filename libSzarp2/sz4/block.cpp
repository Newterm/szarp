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
#include "sz4/block_cache.h"
#include "sz4/block.h"
#include "sz4/block_cache.h"

namespace sz4 {

generic_block::generic_block(block_cache* cache) :
		m_cache(cache) {
	m_cache->add_new_block(this);
}

bool generic_block::ok_to_delete() const {
	return m_refferring_blocks.size() == 0;
}

std::list<generic_block*>::iterator& generic_block::location() {
	return m_block_location;
}

bool generic_block::has_refferring_blocks() const {
	return m_refferring_blocks.size();
}

void generic_block::add_refferring_block(generic_block* block) {
	std::vector<generic_block*>::iterator i = std::find(m_refferring_blocks.begin(), m_refferring_blocks.end(), block);
	if (i != m_refferring_blocks.end()) {
		m_refferring_blocks.push_back(block);
		m_cache->block_updated(this);
	}
}

void generic_block::remove_refferring_block(generic_block* block) {
	std::vector<generic_block*>::iterator i = std::remove(m_refferring_blocks.begin(), m_refferring_blocks.end(), block);
	m_refferring_blocks.erase(i, m_refferring_blocks.end());
	m_cache->block_updated(this);
}

void generic_block::add_refferred_block(generic_block* block) {
	std::vector<generic_block*>::iterator i = std::find(m_refferred_blocks.begin(), m_refferred_blocks.end(), block);
	if (i != m_refferred_blocks.end()) {
		m_refferred_blocks.push_back(block);
		m_cache->block_updated(this);
	}
}

void generic_block::remove_refferred_block(generic_block* block) {
	std::vector<generic_block*>::iterator i = std::remove(m_refferred_blocks.begin(), m_refferred_blocks.end(), block);
	m_refferred_blocks.erase(i, m_refferred_blocks.end());
	m_cache->block_updated(this);
}

void generic_block::remove_from_cache() {
	m_cache->remove_block(this);
}

generic_block::~generic_block() { 
	std::for_each(m_refferred_blocks.begin(), m_refferred_blocks.end(), std::bind2nd(std::mem_fun(&generic_block::remove_refferring_block), this));
	std::for_each(m_refferring_blocks.begin(), m_refferring_blocks.end(), std::bind2nd(std::mem_fun(&generic_block::remove_refferred_block), this));
}

}
