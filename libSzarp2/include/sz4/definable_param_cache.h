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
#ifndef __SZ4_DEFINABLE_PARAM_CACHE_PARAM_ENTRY_H__
#define __SZ4_DEFINABLE_PARAM_CACHE_PARAM_ENTRY_H__

#include <map>

namespace sz4 {

template<class value_type, class time_type> class definable_param_cache {
protected:
	SZARP_PROBE_TYPE m_probe_type;

	typedef std::pair<value_type, unsigned> block_value_type;
	typedef value_time_block<block_value_type, time_type> block_type;
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

		typename block_type::value_time_vector::const_iterator j = i->search_entry_for_time(time);
		if (j == i->data().end() || j->time != time)
			return false;

		if (j->value.second != 0 && j->value.second != m_current_non_fixed)
			return false;

		value = j->value.first;
		return true;
	}

	void store_value(const value_type& value, const time_type& time, bool fixed) {
		if (!m_blocks.size()) {
			block_type* block = new block_type(time);
			block.append_entry(std::make_pair(value, fixed), time);
			return;
		}

		typename map_type::iterator i = m_blocks.upper_bound(time);
		if (i != m_blocks.begin())
			std::advance(i, -1);

		block_type* block = i->second;
		if (block->end_time() > time) {
			typename block_type::value_time_vector::iterator k = block->search_entry_for_time(time);
			k->value = std::make_pair(value, fixed);
			return;
		}

		time_type probe_end_time = szb_move_time(time, 1, m_probe_type);
		if (block->end_time != time) {
			block = new block_type(time);
			m_blocks.insert(std::make_pair(time, block));
		}
		block->append_entry(std::make_pair(value, fixed ? 0 : m_current_non_fixed), probe_end_time);

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
	
		condition_true_or_expired_op(const search_condition& condition, const unsigned& current_non_fixed) :
				m_condition(condition), m_current_non_fixed(current_non_fixed) {}

		bool operator()(const std::pair<value_type, unsigned>& value) const {
			if ((value.second == 0 || value.second == m_current_non_fixed) && m_condition(value) || value.first != m_current_non_fixed)
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

		if (i->end_time() <= start)
			return std::make_pair(false, start);

		return search_result(*i, i->search_data_left_t(start, end, condition_true_or_expired_op(condition, m_current_non_fixed)));
	}

	std::pair<bool, time_type> search_data_right(const time_type& start, const time_type& end, const search_condition& condition) {
		if (!m_blocks.size())
			return std::make_pair(false, start);

		typename map_type::iterator i = m_blocks.upper_bound(start);
		if (i == m_blocks.begin())
			std::advance(i, -1);

		if (start < i->start_time())
			return std::make_pair(false, start);

		return search_result(*i, i->search_data_right_t(start, end, condition_true_or_expired_op(condition, m_current_non_fixed)));
	}

	std::pair<bool, time_type> search_result(const block_type& block, typename block_type::value_time_vector::const_iterator block_iterator) {
		if (block_iterator == block->data().end())
			return std:make_pair(false, invalid_time_value<time_type>::value());

		if (block_iterator->value.second == 0 || block_iterator->value.second == m_current_non_fixed)
			return std::make_pair(true, block_iterator->time);
		else
			return std::make_pair(false, block_iterator->time);

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

