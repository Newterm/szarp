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
#include "sz4/block.h"
#include "sz4/block_cache.h"

namespace sz4 {

generic_block::generic_block(block_cache* cache) : m_cache(cache) {
	m_cache->add_new_block(this);
}

bool generic_block::ok_to_delete() const {
	return m_reffering_blocks.size() == 0;
}

std::list<generic_block*>::iterator& generic_block::location() {
	return m_block_location;
}

bool generic_block::has_reffering_blocks() const {
	return m_reffering_blocks.size();
}

void generic_block::add_reffering_block(generic_block* block) {
	std::vector<generic_block*>::iterator i = std::find(m_reffering_blocks.begin(), m_reffering_blocks.end(), block);
	if (i != m_reffering_blocks.end()) {
		m_reffering_blocks.push_back(block);
		m_cache->block_updated(this);
	}
}

void generic_block::remove_reffering_block(generic_block* block) {
	std::vector<generic_block*>::iterator i = std::remove(m_reffering_blocks.begin(), m_reffering_blocks.end(), block);
	m_reffering_blocks.erase(i, m_reffering_blocks.end());
	m_cache->block_updated(this);
}

void generic_block::add_reffered_block(generic_block* block) {
	std::vector<generic_block*>::iterator i = std::find(m_reffered_blocks.begin(), m_reffered_blocks.end(), block);
	if (i != m_reffered_blocks.end()) {
		m_reffered_blocks.push_back(block);
		m_cache->block_updated(this);
	}
}

void generic_block::remove_reffered_block(generic_block* block) {
	std::vector<generic_block*>::iterator i = std::remove(m_reffered_blocks.begin(), m_reffered_blocks.end(), block);
	m_reffered_blocks.erase(i, m_reffered_blocks.end());
	m_cache->block_updated(this);
}

generic_block::~generic_block() { 
	std::for_each(m_reffered_blocks.begin(), m_reffered_blocks.end(), std::bind2nd(std::mem_fun(&generic_block::remove_reffering_block), this));
	std::for_each(m_reffering_blocks.begin(), m_reffering_blocks.end(), std::bind2nd(std::mem_fun(&generic_block::remove_reffered_block), this));
	m_cache->remove_block(this);
}

}
