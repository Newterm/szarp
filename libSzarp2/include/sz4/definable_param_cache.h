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
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>

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
public:
	SZARP_PROBE_TYPE m_probe_type;

	struct entry_type {
		value_type value;
		unsigned version;
		entry_type() {};
		entry_type(const value_type& _value, 
				const unsigned _version)
				:
				value(_value),
				version(_version) { }
		bool operator==(const entry_type& o) const {
			return value == o.value
				&& version == o.version;
		}
	};

	struct entry_cmp_by_value_and_version {
		bool operator()(const entry_type& e1, const entry_type& e2) const {
			return e1.value == e2.value && e1.version == e2.version;
		}
	};
	
	typedef def_cache_value_type::value_time_type<entry_type, time_type> value_time_type;

	class block_type : public value_time_block<value_time_type, entry_cmp_by_value_and_version> {
		definable_param_cache<value_type, time_type>* m_param_cache;
	public:
		block_type(const time_type& time,
			block_cache* cache,
			definable_param_cache<value_type, time_type>* param_cache)
			:
			value_time_block<value_time_type, entry_cmp_by_value_and_version>(time, cache),
			m_param_cache(param_cache) 
		{}

		~block_type() {
			m_param_cache->remove_block(this);
		}
	};
#if 1
	typedef boost::container::flat_map<time_type,
			block_type*,
			std::less<time_type>,
			allocator_type<std::pair<const time_type, block_type*>
		> > map_type;
#else
	typedef std::map<time_type, block_type*> map_type;
#endif

protected:
	map_type m_blocks;
	unsigned m_current_non_fixed;
	block_cache* m_cache;
public:
	definable_param_cache(SZARP_PROBE_TYPE probe_type, block_cache *cache)
		:
		m_probe_type(probe_type),
		m_current_non_fixed(2),
		m_cache(cache)
	{}

	bool get_value(const time_type& time, value_type& value, bool& fixed) {
		if (!m_blocks.size())
			return false;

		typename map_type::iterator i = m_blocks.upper_bound(time);
		if (i == m_blocks.begin())
			return false;
		std::advance(i, -1);

		typename block_type::value_time_vector::const_iterator j = i->second->search_entry_for_time(time);
		if (j == i->second->data().end())
			return false;

		if (j->value.version != 0 && j->value.version != m_current_non_fixed)
			return false;

		value = j->value.value;
		fixed = j->value.version == 0;
		return true;
	}

	typename map_type::iterator create_new_block(const value_type& value, const time_type& time, unsigned version) {
		block_type* block = new block_type(time, m_cache, this);
		block->append_entry(entry_type(value, version), szb_move_time(time, 1, m_probe_type));
		return m_blocks.insert(std::make_pair(time, block)).first;
	}

	void insert_value_inside_block(block_type* block, const value_type& value, const time_type& time, unsigned version) {
		typename block_type::value_time_vector::iterator i = block->search_entry_for_time(time);
		time_type previous_end_time;

		if (i->value.value == value
				&& i->value.version == version)
			return;

		if (i == block->data().begin())
			previous_end_time = block->start_time();
		else
			previous_end_time = (i - 1)->time;

		if (previous_end_time < time)
			i = block->insert_entry(i,
					entry_type(value, version),
					time) + 1;

		time_type end_time = szb_move_time(time, 1, m_probe_type);
		if (i->time == end_time) {
			i->value = entry_type(value, version);
			block->maybe_merge_3_block_entries(i);
		} else
			block->insert_entry(i, entry_type(value, version), end_time);
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
				i->second->append_entry(entry_type(value, entry_version), szb_move_time(time, 1, m_probe_type));
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

		bool operator()(const entry_type& value) const {
			if ((value.version != 0 && value.version != m_current_non_fixed)
					|| ((value.version == 0 || value.version == m_current_non_fixed)
					&& m_condition(value.value)))
				return true;
			else
				return false;
		}

	};

	std::pair<bool, time_type> search_data_left(const time_type& start, const time_type& end, const search_condition& condition) {
		if (!m_blocks.size())
			return std::make_pair(false, start);

		typename map_type::iterator i = m_blocks.upper_bound(start);
		if (i == m_blocks.begin())
			return std::make_pair(false, start);

		std::advance(i, -1);

		block_type& block = *i->second;
		if (block.end_time() <= start)
			return std::make_pair(false, start);

		typename block_type::value_time_vector::const_iterator j = 
				block.search_data_left_t(start, end, condition_true_or_expired_op(condition, m_current_non_fixed));
		time_type time_found = block.search_result_left(start, j);
		if (!time_trait<time_type>::is_valid(time_found)) {
			time_type prev_time = szb_move_time(block.start_time(), -1, m_probe_type);
			if (prev_time > end)
				return std::make_pair(false, prev_time);
			else
				return std::make_pair(true, time_found);
		} else
			return extract_search_result(time_found, j);
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
		if (!time_trait<time_type>::is_valid(time_found))
			if (block.end_time() < end)
				return std::make_pair(false, block.end_time());
			else
				return std::make_pair(true, time_found);
		else
			return extract_search_result(time_found, j);
	}

	std::pair<bool, time_type> extract_search_result(const time_type& time_found, typename block_type::value_time_vector::const_iterator block_iterator) {
		time_type rounded_time = szb_round_time(time_found, m_probe_type);
		if (block_iterator->value.version == 0
				|| block_iterator->value.version == m_current_non_fixed)
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
				if (j->value.version != 0)
					j->value.version = 1;
			}
		}
	}

	void remove_block(block_type *block) {
		for (typename map_type::iterator i =  m_blocks.begin();
				i != m_blocks.end();
				i++)
			if (i->second == block) {
				m_blocks.erase(i);
				break;
			}
	}

	~definable_param_cache() {
		while (m_blocks.size()) {
			block_type *block = m_blocks.rbegin()->second;
			delete block;
		}
	}
};

}

#endif

