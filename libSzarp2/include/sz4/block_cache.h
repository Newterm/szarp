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
#ifndef __SZ4_BLOCK_CACHE_H__
#define __SZ4_BLOCK_CACHE_H__

#include <list>

namespace sz4 {

class block_cache {
	std::list<generic_block*> m_blocks;
	std::list<generic_block*>::iterator m_list_separator_position;
	size_t m_cache_size;
public:
	block_cache();
	size_t cache_size() const;
	void add_new_block(generic_block* block);
	void remove_block(generic_block* block);
	void block_size_changed(generic_block* block, size_t previous_size);
	void block_updated(generic_block* block);
	void remove(size_t size);
};

}

#endif
