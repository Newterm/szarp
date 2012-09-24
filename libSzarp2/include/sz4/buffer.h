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
#ifndef __SZ4_BUFFER_H__
#define __SZ4_BUFFER_H__

#include "config.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "szbase/szbparamobserver.h"
#include "szbase/szbparammonitor.h"

#include "sz4/defs.h"
#include "sz4/time.h"
#include "sz4/block.h"
#include "sz4/path.h"
#include "sz4/load_file_locked.h"

namespace sz4 {

class generic_param_entry : public SzbParamObserver {
protected:
	TParam* m_param;
	const boost::filesystem::wpath m_param_dir;
	std::vector<std::string> m_paths_to_update;
	boost::mutex m_mutex;
public:
	generic_param_entry(TParam* param, const boost::filesystem::wpath& param_dir) : m_param(param), m_param_dir(param_dir) {}
	TParam* get_param() const { return m_param; }
	virtual void get_weighted_sum(const second_time_t& start, const second_time_t& end, weighted_sum<short, second_time_t>& wsum) = 0;
	virtual void get_weighted_sum(const second_time_t& start, const second_time_t& end, weighted_sum<int, second_time_t>& wsum) = 0;
	virtual void get_weighted_sum(const second_time_t& start, const second_time_t& end, weighted_sum<float, second_time_t>& wsum) = 0;
	virtual void get_weighted_sum(const second_time_t& start, const second_time_t& end, weighted_sum<double , second_time_t>& wsum) = 0;
	virtual void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, weighted_sum<short, nanosecond_time_t>& wsum) = 0;
	virtual void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, weighted_sum<int, nanosecond_time_t>& wsum) = 0;
	virtual void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, weighted_sum<float, nanosecond_time_t>& wsum) = 0;
	virtual void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, weighted_sum<double, nanosecond_time_t>& wsum) = 0;

	virtual second_time_t search_data_right(const second_time_t& start, const second_time_t& end, const search_condition& condition) = 0;
	virtual second_time_t search_data_left(const second_time_t& start, const second_time_t& end, const search_condition& condition) = 0;
	virtual nanosecond_time_t search_data_right(const nanosecond_time_t& start, const nanosecond_time_t& end, const search_condition& condition) = 0;
	virtual nanosecond_time_t search_data_left(const nanosecond_time_t& start, const nanosecond_time_t& end, const search_condition& condition) = 0;

	virtual void refresh_file(const std::string& path) = 0;

	void register_at_monitor(SzbParamMonitor* monitor);
	void deregister_from_monitor(SzbParamMonitor* monitor);
	void param_data_changed(TParam*, const std::string& path);
	virtual ~generic_param_entry() {}
};

template<class V, class T> class block_entry {
	concrete_block<V, T> m_block;
	bool m_needs_refresh;
	const boost::filesystem::wpath m_block_path;
public:
	block_entry(const T& start_time, const std::wstring& block_path) :
		m_block(start_time), m_needs_refresh(true), m_block_path(block_path) {}

	void get_weighted_sum(const T& start, const T& end, weighted_sum<V, T>& wsum) {
		m_block.get_weighted_sum(start, end, wsum);
	}

	T search_data_right(const T& start, const T& end, const search_condition& condition) {
		return m_block.search_data_right(start, end, condition);
	}

	T search_data_left(const T& start, const T& end, const search_condition& condition) {
		return m_block.search_data_left(start, end, condition);
	}

	void refresh_if_needed() {
		if (!m_needs_refresh)
			return;

		std::vector<value_time_pair<V, T> > values;
		size_t size = boost::filesystem::file_size(m_block_path);
		values.resize(size / sizeof(value_time_pair<V, T>));

		if (load_file_locked(m_block_path, (char*) &values[0], values.size() * sizeof(value_time_pair<V, T>)))
			m_block.set_data(values);
		m_needs_refresh = false;
	}

	void set_needs_refresh() {
		m_needs_refresh = true;
	}

	concrete_block<V, T>& block() { return m_block; }
};

namespace param_buffer_type_converion_helper {

template<class IV, class IT, class OV, class OT> class helper {
	weighted_sum<IV, IT> m_temp;
	weighted_sum<OV, OT>& m_sum;
public:
	helper(weighted_sum<OV, OT>& sum) : m_sum(sum) {}
	weighted_sum<IV, IT>& sum() { return m_temp; }
	void convert() {
		m_sum.sum() = m_temp.sum();
		m_sum.weight() = m_temp.weight();
		m_sum.no_data_weight() = m_temp.no_data_weight();
	}
};

template<class V, class T> class helper<V, T, V, T> {
	weighted_sum<V, T>& m_sum;
public:
	helper(weighted_sum<V, T>& sum) : m_sum(sum) {}
	weighted_sum<V, T>& sum() { return m_sum; }
	void convert() { }
};

}

template<class V, class T> class param_entry_in_buffer : public generic_param_entry {
	typedef std::map<T, block_entry<V, T>*> map_type;

	bool m_refresh_file_list;
	map_type m_blocks;

	template<class RV, class RT> void get_weighted_sum_templ(const T& start, const T& end, weighted_sum<RV, RT>& sum)  {
		param_buffer_type_converion_helper::helper<V, T, RV, RT> helper(sum);
		get_weighted_sum_impl(start, end, helper.sum());
		helper.convert();
	}

	void get_weighted_sum_impl(const T& start, const T& end, weighted_sum<V, T>& sum)  {
		refresh_if_needed();
		//search for the first block that has starting time bigger than
		//start_time
		if (m_blocks.size() == 0) {
			//no such block - nodata
			sum.no_data_weight() += end - start;
			return;
		}

		T current(start);
		typename map_type::iterator i = m_blocks.upper_bound(start);
		//if not first - back one off
		if (i != m_blocks.begin())
			std::advance(i, -1);
		do {
			block_entry<V, T>* entry = i->second;

			if (end < entry->block().start_time())
				break;

			if (current < entry->block().start_time()) {
				sum.no_data_weight() += entry->block().start_time() - current;
				current = i->first;
			}

			entry->refresh_if_needed();
			if (current < entry->block().end_time()) {
				T end_for_block = std::min(end, entry->block().end_time());
				entry->get_weighted_sum(current, end_for_block, sum);

				current = end_for_block;
			}

			std::advance(i, 1);
		} while (i != m_blocks.end());

		if (current < end)
			sum.no_data_weight() += end - current;

	}

	T search_data_right_impl(const T& start, const T& end, const search_condition& condition) {
		refresh_if_needed();
		if (m_blocks.size() == 0)
			return invalid_time_value<T>::value;

		typename map_type::iterator i = m_blocks.upper_bound(start);
		//if not first - step one back
		if (i != m_blocks.begin())
			std::advance(i, -1);

		while (i != m_blocks.end()) {
			block_entry<V, T>* entry = i->second;
			if (entry->block().start_time() >= end)
				break;

			entry->refresh_if_needed();
			T t = entry->search_data_right(start, end, condition);
			if (invalid_time_value<T>::is_valid(t))
				return t;

			std::advance(i, 1);
		}

		return invalid_time_value<T>::value;
	}

	T search_data_left_impl(const T& start, const T& end, const search_condition& condition) {
		refresh_if_needed();
		if (m_blocks.size() == 0)
			return invalid_time_value<T>::value;

		typename map_type::iterator i = m_blocks.upper_bound(start);
		if (i != m_blocks.begin())
			std::advance(i, -1);

		while (true) {
			block_entry<V, T>* entry = i->second;

			entry->refresh_if_needed();
			if (entry->block().end_time() <= end)
				break;

			T t = entry->search_data_left(start, end, condition);
			if (invalid_time_value<T>::is_valid(t))
				return t;

			if (i == m_blocks.begin())
				break;
			std::advance(i, -1);
		}

		return invalid_time_value<T>::value;
	}

	void refresh_if_needed() {
		{
			boost::mutex::scoped_lock lock(m_mutex);
			for (std::vector<std::string>::iterator i = m_paths_to_update.begin(); i != m_paths_to_update.end(); i++) {
				refresh_file(*i);
			}
			m_paths_to_update.clear();
		}
		if (m_refresh_file_list) {
			try {
				refresh_file_list();
			} catch (boost::filesystem::wfilesystem_error&) {}
			m_refresh_file_list = false;
		}
	}

	
	void refresh_file(const std::string& path) {
		T time = path_to_date<T>(path);
		if (time == invalid_time_value<T>::value)
			return;

		typename map_type::iterator i = m_blocks.find(time);
		if (i != m_blocks.end())
			i->second->set_needs_refresh();
		else
			m_refresh_file_list = true;
	}

	void refresh_file_list() {
		namespace fs = boost::filesystem;
		
		for (fs::wdirectory_iterator i(m_param_dir); 
				i != fs::wdirectory_iterator(); 
				i++) {

			std::wstring file_path = i->path().file_string();

			T file_time = path_to_date<T>(file_path);
			if (!invalid_time_value<T>::is_valid(file_time))
				continue;

			if (m_blocks.find(file_time) != m_blocks.end())
				continue;

			m_blocks.insert(std::make_pair(file_time, new block_entry<V, T>(file_time, file_path)));
		}
	}


public:
	param_entry_in_buffer(TParam* param, const boost::filesystem::wpath& base_dir) :
		generic_param_entry(param, base_dir / param->GetSzbaseName()), m_refresh_file_list(true)
	{ }

	void get_weighted_sum(const second_time_t& start, const second_time_t& end, weighted_sum<short, second_time_t>& wsum) {
		get_weighted_sum_templ(start, end, wsum);
	}

	void get_weighted_sum(const second_time_t& start, const second_time_t& end, weighted_sum<int, second_time_t>& wsum) {
		get_weighted_sum_templ(start, end, wsum);
	}

	void get_weighted_sum(const second_time_t& start, const second_time_t& end, weighted_sum<float, second_time_t>& wsum) {
		get_weighted_sum_templ(start, end, wsum);
	}

	void get_weighted_sum(const second_time_t& start, const second_time_t& end, weighted_sum<double , second_time_t>& wsum) {
		get_weighted_sum_templ(start, end, wsum);
	}

	void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, weighted_sum<short, nanosecond_time_t>& wsum) {
		get_weighted_sum_templ(start, end, wsum);
	}

	void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, weighted_sum<int, nanosecond_time_t>& wsum) {
		get_weighted_sum_templ(start, end, wsum);
	}

	void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, weighted_sum<float, nanosecond_time_t>& wsum) {
		get_weighted_sum_templ(start, end, wsum);
	}

	void get_weighted_sum(const nanosecond_time_t& start, const nanosecond_time_t& end, weighted_sum<double, nanosecond_time_t>& wsum) {
		get_weighted_sum_templ(start, end, wsum);
	}

	second_time_t search_data_right(const second_time_t& start, const second_time_t& end, const search_condition& condition) {
		return search_data_right_templ(start, end, condition);
	}

	second_time_t search_data_left(const second_time_t& start, const second_time_t& end, const search_condition& condition) {
		return search_data_left_templ(start, end, condition);
	}

	nanosecond_time_t search_data_right(const nanosecond_time_t& start, const nanosecond_time_t& end, const search_condition& condition) {
		return search_data_right_templ(start, end, condition);
	}

	nanosecond_time_t search_data_left(const nanosecond_time_t& start, const nanosecond_time_t& end, const search_condition& condition) {
		return search_data_left_templ(start, end, condition);
	}

	template<class RT> RT search_data_right_templ(const RT& start, const RT &end, const search_condition& condition) {
		return RT(search_data_right_impl(T(start), T(end), condition));
	}

	template<class RT> RT search_data_left_templ(const RT& start, const RT &end, const search_condition& condition) {
		return RT(search_data_left_impl(T(start), T(end), condition));
	}

	virtual ~param_entry_in_buffer() {
		for (typename map_type::iterator i = m_blocks.begin(); i != m_blocks.end(); i++)
			delete i->second;
	}
};

class buffer {
	SzbParamMonitor* m_param_monitor;
	boost::filesystem::wpath m_buffer_directory;
	std::vector<generic_param_entry*> m_param_ents;
public:
	buffer(SzbParamMonitor* param_monitor, const std::wstring& buffer_directory)
		: m_param_monitor(param_monitor), m_buffer_directory(buffer_directory) {}

	generic_param_entry* get_param_entry(TParam* param) {
		if (m_param_ents.size() <= param->GetParamId())
			m_param_ents.resize(param->GetParamId() + 1, NULL);

		generic_param_entry* entry = m_param_ents[param->GetParamId()];
		if (entry == NULL) {
			entry = m_param_ents[param->GetParamId()] = create_param_entry(param);
			entry->register_at_monitor(m_param_monitor);
		}
		return entry;
	}

	template<class T, class V> void get_weighted_sum(TParam* param, const T& start, const T &end, weighted_sum<V, T> &wsum) {
		get_param_entry(param)->get_weighted_sum(start, end, wsum);
	}

	template<class T> T search_data_right(TParam* param, const T& start, const T& end, const search_condition &condition) {
		return get_param_entry(param)->search_data_right(start, end, condition);
	}

	template<class T> T search_data_left(TParam* param, const T& start, const T& end, const search_condition &condition) {
		return get_param_entry(param)->search_data_left(start, end, condition);
	}

	generic_param_entry* create_param_entry(TParam* param);

	void remove_param(TParam* param);

	~buffer() {
		for (std::vector<generic_param_entry*>::iterator i = m_param_ents.begin(); i != m_param_ents.end(); i++) {
			if (*i)
				(*i)->deregister_from_monitor(m_param_monitor);
			delete *i;
		}
	}
};

}

#endif
