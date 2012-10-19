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

	typename map_type::iterator create_new_block(const value_type& value, const time_type& time, unsigned version) {
		block_type* block = new block_type(time);
		block->append_entry(std::make_pair(value, version), szb_move_time(time, 1, m_probe_type));
		return m_blocks.insert(std::make_pair(time, block)).first;
	}

	void maybe_merge_3_block_entries(block_type* block,  typename block_type::value_time_vector::iterator i) {
		if (i != block->data().begin() && (i + 1) != block->data().end() && (i - 1)->value == i->value && i->value == (i + 1)->value) {
			(i - 1)->time = (i + 1)->time;
			block->data().erase(i, i + 2);
		}
	}

	void insert_value_inside_block(block_type* block, const value_type& value, const time_type& time, unsigned version) {
		typename block_type::value_time_vector::iterator i = block->search_entry_for_time(time);
		time_type previous_end_time;

		if (i->value == std::make_pair(value, version))
			return;

		if (i == block->data().begin())
			previous_end_time = block->start_time();
		else
			previous_end_time = (i - 1)->time;

		if (previous_end_time < time)
			i = block->insert_entry(i, i->value, time) + 1;

		time_type end_time = szb_move_time(time, 1, m_probe_type);
		if (i->time == end_time) {
			i->value = std::make_pair(value, version);
			maybe_merge_3_block_entries(block, i);
		} else
			block->insert_entry(i, std::make_pair(value, version), end_time);
	}

	void maybe_merge_block_with_next_one(typename map_type::iterator i) {
		block_type* block = i->second;
		std::advance(i, 1);
		if (i != m_blocks.end() && i->first == block->end_time()) {
			block_type* block_n = i->second;
			block->append_entries(block_n->data().begin(), block_n->data().end());

			m_blocks.erase(i);
			delete block_n;
		}
	}

	void store_value(const value_type& value, const time_type& time, bool fixed) {
		unsigned entry_version = fixed ? 0 : m_current_non_fixed;
		
		if (!m_blocks.size()) {
			create_new_block(value, time, entry_version);
			return;
		}

		typename map_type::iterator i = m_blocks.upper_bound(time);
		if (i == m_blocks.begin())
			i = create_new_block(value, time, entry_version);
		else {
			std::advance(i, -1);
			if (i->second->end_time() < time)
				i = create_new_block(value, time, entry_version);
			else if (i->second->end_time() == time)
				i->second->append_entry(std::make_pair(value, entry_version), szb_move_time(time, 1, m_probe_type));
			else
				insert_value_inside_block(i->second, value, time, entry_version);
		}

		maybe_merge_block_with_next_one(i);
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
		if (!invalid_time_value<time_type>::is_valid(time_found)) {
			if (block.start_time() > time_type(szb_move_time(end, 1, m_probe_type))) 
				return std::make_pair(false, szb_move_time(block.start_time(), -1, m_probe_type));
			else
				return std::make_pair(true, time_found);
		} else
			return search_result(time_found, j);
	}

	std::pair<bool, time_type> search_data_right(const time_type& start, const time_type& end, const search_condition& condition) {
		if (!m_blocks.size())
			return std::make_pair(false, start);

		typename map_type::iterator i = m_blocks.upper_bound(start);
		if (i != m_blocks.begin())
			std::advance(i, -1);

		block_type& block = *i->second;
		if (start < block.start_time())
			return std::make_pair(false, start);

		typename block_type::value_time_vector::const_iterator j = 
				block.search_data_right_t(start, end, condition_true_or_expired_op(condition, m_current_non_fixed));
		time_type time_found = block.search_result_right(start, j);
		if (!invalid_time_value<time_type>::is_valid(time_found))
			if (block.end_time() < end)
				return std::make_pair(false, block.end_time());
			else
				return std::make_pair(true, time_found);
		else
			return search_result(time_found, j);
	}

	std::pair<bool, time_type> search_result(const time_type& time_found, typename block_type::value_time_vector::const_iterator block_iterator) {
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
			typename block_type::value_time_vector& data = i->second->data();
			for (typename block_type::value_time_vector::iterator j = data.begin(); j != data.end(); j++) {
				if (j->value.second != 0)
					j->value.second = 1;
			}
		}
	}

	~definable_param_cache() {
		for (typename map_type::iterator i = m_blocks.begin(); i != m_blocks.end(); i++)
			delete i->second;
	}
};

}

#endif

