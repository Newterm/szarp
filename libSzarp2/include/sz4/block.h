/* 
  SZARP: SCADA software 
  

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
#ifndef __SZ4_BLOCK_H__
#define __SZ4_BLOCK_H__

#include <algorithm>
#include <iterator>
#include <list>

#include <boost/intrusive/list.hpp>

#include "sz4/defs.h"
#include "sz4/time.h"

namespace sz4 {

template<class pair_type> pair_type make_value_time_pair(const typename pair_type::value_type& v, const typename pair_type::time_type& t) {
	pair_type pair;
	pair.value = v;
	pair.time = t;
	return pair;
}

template<class time_type, class pair_type> bool cmp_time(const time_type& t, const pair_type& p) {
	return t < p.time;
}

template<class iterator, class time_type> iterator search_entry_for_time(iterator begin, iterator end, const time_type& t) {
	typedef typename iterator::value_type pair_type;

	return std::upper_bound(begin, end, t, cmp_time<time_type, pair_type>);
}

template<class iterator, class time_type, class value_type> 
void get_weighted_sum(iterator begin, iterator end,
		const time_type& start_time, const time_type &end_time,
		weighted_sum<value_type, time_type>& r)
{
	bool done = false;
	time_type prev_time = start_time;
	auto i = search_entry_for_time(begin, end, start_time);
	while (!done && i != end) {
		typename time_difference<time_type>::type time_diff;
		if (i->time >= end_time) {
			time_diff = end_time - prev_time;
			done = true;
		} else {
			time_diff = i->time - prev_time;
		}
		prev_time = i->time;

		if (!value_is_no_data(i->value)) {
			r.add(i->value, time_diff);
		} else {
			r.add_no_data_weight(time_diff);
		}

		std::advance(i, 1);
	}
}

template<class iterator, class time_type, class search_op>
iterator search_data_left_t(
		iterator begin, iterator end,
		const time_type& start_time, const time_type& end_time,
		const search_op &condition) {

	if (begin == end)
		return end;

	auto i = search_entry_for_time(begin, end, start_time);
	if (i == end)
		std::advance(i, -1);

	while (true) {
		if (i->time <= end_time)
			break;

		if (condition(i->value))
			return i;

		if (i == begin)
			break;

		std::advance(i, -1);
	}
	return end;
}

template<class iterator, class time_type, class search_op>
iterator search_data_right_t(
		iterator begin, iterator end,
		const time_type& start_time, const time_type& end_time,
		const search_op &condition) {

	auto i = search_entry_for_time(begin, end, start_time);
	while (i != end) {
		if (condition(i->value))
			return i;

		if (i->time >= end_time)
			break;
		std::advance(i, 1);
	}
	return end;
}

class block_cache;
class generic_block {
protected:
	block_cache* m_cache;
public:
	generic_block(block_cache* cache);

	block_cache* cache();

	virtual size_t block_size() const = 0;

	void block_data_updated(size_t previous_size);

	void remove_from_cache(size_t block_size);

	virtual ~generic_block();

	boost::intrusive::list_member_hook<> m_list_entry;
};

class cache_block_size_updater {
	block_cache* m_cache;
	generic_block* m_block;
	size_t m_previous_size;
public:
	cache_block_size_updater(generic_block* block);
	~cache_block_size_updater();
};

typedef boost::intrusive::list<
	generic_block, 
	boost::intrusive::member_hook<generic_block, boost::intrusive::list_member_hook<>, &generic_block::m_list_entry>
	> generic_block_list;


template<class T> struct empty_merge {
	void operator()(T& t, T& t2, T& t3) const {}
};

template<
	class value_time_type,
	class value_cmp = std::equal_to<typename value_time_type::value_type>,
	class value_merge = empty_merge<typename value_time_type::value_type>
> class value_time_block : public generic_block {
public:
	typedef typename value_time_type::value_type value_type;
	typedef typename value_time_type::time_type time_type;

	typedef std::vector<value_time_type> value_time_vector;

	value_time_block(const time_type& time,
		block_cache* cache)
		:
		generic_block(cache),
		m_start_time(time)
	{}

	const time_type& start_time() const { return m_start_time; }
	const time_type end_time() const {
		if (m_data.size())
			return m_data[m_data.size() - 1].time;
		else
			return m_start_time;
	}

	typename value_time_vector::iterator search_entry_for_time(const time_type& time) {
		return sz4::search_entry_for_time(m_data.begin(), m_data.end(), time);
	}

	typename value_time_vector::const_iterator search_entry_for_time(const time_type& time) const {
		return sz4::search_entry_for_time(m_data.begin(), m_data.end(), time);
	}

	const value_time_vector& data() const {
		return m_data;
	}

	value_time_vector& data() {
		return m_data;
	}

	size_t block_size() const {
		return m_data.size() * (sizeof(value_type) + sizeof(time_type));
	}

	template<class search_op> typename value_time_vector::const_iterator search_data_right_t(const time_type& start, const time_type& end, const search_op &condition) {
	
		return sz4::search_data_right_t(m_data.begin(), m_data.end(), start, end, condition);	

	}

	time_type search_result_right(const time_type& start, typename value_time_vector::const_iterator i) {
		if (i == m_data.end())
			return time_trait<time_type>::invalid_value;

		if (i == m_data.begin())
			return std::max(start, m_start_time);
		else  {
			std::advance(i, -1);
			return std::max(i->time, start);
		}
	}

	time_type search_result_left(const time_type& start, typename value_time_vector::const_iterator i) {
		if (i != m_data.end())
			return std::min(time_just_before(i->time), start);
		else 
			return time_trait<time_type>::invalid_value;
	}


	template<class search_op> typename value_time_vector::const_iterator search_data_left_t(const time_type& start, const time_type& end, const search_op &condition) {
		if (m_data.size() == 0)
			return m_data.end();

		return sz4::search_data_left_t(m_data.begin(), m_data.end(), start, end, condition);	
	}

	void append_entry(const value_type& value, const time_type& time) {
		cache_block_size_updater _updater(this);
		if (!m_data.size())
			m_data.push_back(make_value_time_pair<value_time_type>(value, time));
		else {
			value_time_type& last_value_time = *m_data.rbegin();
			if (last_value_time.value == value) {
				last_value_time.time = time;
			} else {
				m_data.push_back(make_value_time_pair<value_time_type>(value, time));
			}
		}
	}

	void append_entries(typename value_time_vector::iterator begin, typename value_time_vector::iterator end) {
		if (begin == end)
			return;

		append_entry(begin->value, begin->time);

		cache_block_size_updater _updater(this);
		m_data.insert(m_data.end(), begin + 1, end);
	}

	typename value_time_vector::iterator insert_entry(typename value_time_vector::iterator i, const value_type& value, const time_type& time) {
		cache_block_size_updater _updater(this);
		return m_data.insert(i, make_value_time_pair<value_time_type>(value, time));
	}

	void set_data(value_time_vector& data) {
		cache_block_size_updater _updater(this);
		m_data.swap(data);
	}

	void set_start_time(const time_type& time) {
		m_start_time = time;
	}

	void maybe_merge_3_block_entries(typename value_time_vector::iterator i) {
		cache_block_size_updater _updater(this);
		if (i != m_data.begin()
				&& (i + 1) != m_data.end()
				&& value_cmp()((i - 1)->value, i->value)
				&& value_cmp()(i->value, (i + 1)->value))
		{
			(i - 1)->time = (i + 1)->time;
			value_merge()((i - 1)->value, i->value, (i + 1)->value);
			m_data.erase(i, i + 2);
		}
	}

	virtual ~value_time_block() {
		remove_from_cache(block_size());
	}

protected:
	value_time_vector m_data;
	time_type m_start_time;
};

template<class value_type, class time_type> class concrete_block : public value_time_block<value_time_pair<value_type, time_type> > {
public:
	typedef value_time_block<value_time_pair<value_type, time_type> > block_type;

	concrete_block(const time_type& start_time,
			block_cache* cache)
		:
			block_type(start_time, cache)
	{}

	void get_weighted_sum(const time_type& start_time, const time_type &end_time, weighted_sum<value_type, time_type>& r) {
		if (start_time >= end_time)
			return;

		this->m_cache->block_touched(*this);

		sz4::get_weighted_sum(this->m_data.begin(), this->m_data.end(), start_time, end_time, r);
	}

	time_type search_data_right(const time_type& start, const time_type& end, const search_condition &condition) {
		auto i = this->search_data_right_t(start, end, condition);
		return this->search_result_right(start, i);
	}

	time_type search_data_left(const time_type& start, const time_type& end, const search_condition &condition) {
		auto i = this->search_data_left_t(start, end, condition);
		return this->search_result_left(start, i);
	}

};


}

#endif
