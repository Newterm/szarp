#ifndef __SZ4_DEFINABLE_PARAM_CACHE_PARAM_ENTRY_H__
#define __SZ4_DEFINABLE_PARAM_CACHE_PARAM_ENTRY_H__
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

#include <map>

namespace sz4 {

namespace def_cache_value_type {

template<class V, class T> struct value_time_type {
	typedef V value_type;
	typedef T time_type;
	value_type value;
	time_type time;
};

}


template<class value_type, class time_type> class definable_param_cache {
protected:
	SZARP_PROBE_TYPE m_probe_type;

	typedef std::pair<value_type, unsigned> value_version_type;
	typedef def_cache_value_type::value_time_type<value_version_type, time_type> value_time_type;

	typedef value_time_block<value_time_type> block_type;
	typedef std::map<time_type, block_type*> map_type;

	map_type m_blocks;
	unsigned m_current_non_fixed;			
public:
	definable_param_cache(SZARP_PROBE_TYPE probe_type) :
		m_probe_type(probe_type), m_current_non_fixed(2)
	{}

	bool get_value(const time_type& time, value_type& value) {
		if (!m_blocks.size())
			return false;

		typename map_type::iterator i = m_blocks.upper_bound(time);
		if (i != m_blocks.begin())
			std::advance(i, -1);

		typename block_type::value_time_vector::const_iterator j = i->second->search_entry_for_time(time);
		if (j == i->second->data().end())
			return false;

		if (j->value.second != 0 && j->value.second != m_current_non_fixed)
			return false;

		value = j->value.first;
		return true;
	}

	void store_value(const value_type& value, const time_type& time, bool fixed) {
		unsigned entry_version = fixed ? 0 : m_current_non_fixed;
		
		if (!m_blocks.size()) {
			block_type* block = new block_type(time);
			block->append_entry(std::make_pair(value, entry_version), szb_move_time(time, 1, m_probe_type));

			m_blocks.insert(std::make_pair(time, block));
			return;
		}

		typename map_type::iterator i = m_blocks.upper_bound(time);
		if (i != m_blocks.begin())
			std::advance(i, -1);

		block_type* block = i->second;
		if (block->end_time() > time) {
			typename block_type::value_time_vector::iterator k = block->search_entry_for_time(time);
			k->value = std::make_pair(value, entry_version);
			return;
		}

		time_type probe_end_time = szb_move_time(time, 1, m_probe_type);
		if (block->end_time() != time) {
			block = new block_type(time);
			m_blocks.insert(std::make_pair(time, block));
		}
		block->append_entry(std::make_pair(value, entry_version), probe_end_time);

		std::advance(i, 1);
		if (i != m_blocks.end() && i->first == probe_end_time) {
			block_type* block_n = i->second;
			block->append_entries(block_n->data());

			m_blocks.erase(i);
			delete block_n;
		}
	}

	class condition_true_or_expired_op {
		const search_condition& m_condition;
		const unsigned& m_current_non_fixed;
	public:	
		condition_true_or_expired_op(const search_condition& condition, const unsigned& current_non_fixed) :
				m_condition(condition), m_current_non_fixed(current_non_fixed) {}

		bool operator()(const std::pair<value_type, unsigned>& value) const {
			if ((value.second != 0 && value.second != m_current_non_fixed)
					|| ((value.second == 0 || value.second == m_current_non_fixed) && m_condition(value.first)))
				return true;
			else
				return false;
		}

	};

	std::pair<bool, time_type> search_data_left(const time_type& start, const time_type& end, const search_condition& condition) {
		if (!m_blocks.size())
			return std::make_pair(false, start);

		typename map_type::iterator i = m_blocks.upper_bound(start);
		if (i != m_blocks.begin())
			std::advance(i, -1);

		block_type& block = *i->second;
		if (block.end_time() <= start)
			return std::make_pair(false, start);

		typename block_type::value_time_vector::const_iterator j = 
				block.search_data_left_t(start, end, condition_true_or_expired_op(condition, m_current_non_fixed));
		time_type time_found = block.search_result_left(start, j);
		return search_result(time_found, j);
	}

	std::pair<bool, time_type> search_data_right(const time_type& start, const time_type& end, const search_condition& condition) {
		if (!m_blocks.size())
			return std::make_pair(false, start);

		typename map_type::iterator i = m_blocks.upper_bound(start);
		if (i == m_blocks.begin())
			std::advance(i, -1);

		block_type& block = *i->second;
		if (start < block->start_time())
			return std::make_pair(false, start);

		typename block_type::value_time_vector::const_iterator j = 
				block.search_data_right_t(start, end, condition_true_or_expired_op(condition, m_current_non_fixed));
		time_type time_found = block.search_result_right(start, j);
		return search_result(time_found, j);
	}

	std::pair<bool, time_type> search_result(const time_type& time_found, typename block_type::value_time_vector::const_iterator block_iterator) {
		if (!invalid_time_value<time_type>::is_valid(time_found))
			return std::make_pair(false, time_found);

		time_type rounded_time = szb_round_time(time_found, m_probe_type);
		if (block_iterator->value.second == 0 || block_iterator->value.second == m_current_non_fixed)
			return std::make_pair(true, rounded_time);
		else
			return std::make_pair(false, rounded_time);

	}

	void invalidate_non_fixed_values() {
		if (++m_current_non_fixed != 0)
			return;

		m_current_non_fixed = 2;
		for (typename map_type::iterator i = m_blocks.begin(); i != m_blocks.end(); i++) {
			typename block_type::value_time_vector& data = i->value->data();
			for (typename block_type::value_time_vector::iterator j = data.begin(); j != data.end(); j++) {
				if (j->value.second != 0)
					j->value.second = 1;
			}
		}
	}
};

}

#endif

