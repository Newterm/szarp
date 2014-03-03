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
#ifndef __SZ4_REAL_PARAM_ENTRY_H__
#define __SZ4_REAL_PARAM_ENTRY_H__

namespace sz4 {

template<class V, class T, class types> class real_param_entry_in_buffer;

template<class V, class T, class types> class file_block_entry : public block_entry<V, T> {
	const boost::filesystem::wpath m_block_path;
	real_param_entry_in_buffer<V, T, types>* m_param_entry;
public:
	file_block_entry(const T& start_time,
			const std::wstring& block_path,
			block_cache* cache,
			real_param_entry_in_buffer<V, T, types>* param_entry)
		:
			block_entry<V, T>(start_time, cache),
			m_block_path(block_path),
			m_param_entry(param_entry)
		{}

	void refresh_if_needed() {
		if (!this->m_needs_refresh)
			return;

		std::vector<value_time_pair<V, T> > values;
		size_t size = boost::filesystem::file_size(m_block_path);
		values.resize(size / sizeof(value_time_pair<V, T>));

		if (load_file_locked(m_block_path, (char*) &values[0], values.size() * sizeof(value_time_pair<V, T>)))
			this->m_block.set_data(values);
		this->m_needs_refresh = false;

	}

	~file_block_entry() {
		m_param_entry->remove_block(this);
	}
};


template<class V, class T, class types> class real_param_entry_in_buffer {
public:
	typedef file_block_entry<V, T, types> file_block_entry_type;
	typedef std::map<T, file_block_entry_type*> map_type;
private:
	base_templ<types>* m_base;
	map_type m_blocks;

	TParam* m_param;
	const boost::filesystem::wpath m_param_dir;
	std::vector<std::string> m_paths_to_update;

	boost::mutex m_mutex;

	bool m_refresh_file_list;
public:
	real_param_entry_in_buffer(base_templ<types> *_base, TParam* param, const boost::filesystem::wpath& param_dir) : m_base(_base), m_param(param), m_param_dir(param_dir), m_refresh_file_list(true)
		{}
	void get_weighted_sum_impl(const T& start, const T& end, SZARP_PROBE_TYPE, weighted_sum<V, T>& sum)  {
		refresh_if_needed();
		//search for the first block that has starting time bigger than
		//start_time
		if (m_blocks.size() == 0) {
			//no such block - nodata
			sum.add_no_data_weight(end - start);
			sum.set_fixed(false);
			return;
		}

		T current(start);
		typename map_type::iterator i = m_blocks.upper_bound(start);
		//if not first - back one off
		if (i != m_blocks.begin())
			std::advance(i, -1);
		do {
			file_block_entry_type* entry = i->second;

			if (end < entry->block().start_time()) {
				sum.add_no_data_weight(end - current);
				return;
			}

			if (current < entry->block().start_time()) {
				sum.add_no_data_weight(entry->block().start_time() - current);
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

		if (current < end) {
			sum.add_no_data_weight(end - current);
			sum.set_fixed(false);
		}
	}

	T search_data_right_impl(const T& start, const T& end, SZARP_PROBE_TYPE, const search_condition& condition) {
		refresh_if_needed();
		if (m_blocks.size() == 0)
			return invalid_time_value<T>::value;

		typename map_type::iterator i = m_blocks.upper_bound(start);
		//if not first - step one back
		if (i != m_blocks.begin())
			std::advance(i, -1);

		while (i != m_blocks.end()) {
			file_block_entry_type* entry = i->second;
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

	T search_data_left_impl(const T& start, const T& end, SZARP_PROBE_TYPE, const search_condition& condition) {
		refresh_if_needed();
		if (m_blocks.size() == 0)
			return invalid_time_value<T>::value;

		typename map_type::iterator i = m_blocks.upper_bound(start);
		if (i != m_blocks.begin())
			std::advance(i, -1);
		else if (i->first > start)
			return invalid_time_value<T>::value;

		while (true) {
			file_block_entry_type* entry = i->second;

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

	void remove_block(file_block_entry_type* block) {
		for (typename map_type::iterator i = m_blocks.begin();
				i != m_blocks.end();
				i++)
			if (i->second == block) {
				m_blocks.erase(i);
				break;
			}
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

			m_blocks.insert(std::make_pair(file_time, new file_block_entry_type(file_time, file_path, m_base->cache(), this)));
		}
	}

	T get_first_time() {
		refresh_if_needed();
		if (m_blocks.size() == 0)
			return invalid_time_value<T>::value;
		else {
			file_block_entry_type* entry = m_blocks.rbegin()->second;
			entry->refresh_if_needed();
			return entry->block().end_time();
		}
	}

	T get_last_time() {
		refresh_if_needed();
		if (m_blocks.size() == 0)
			return invalid_time_value<T>::value;
		else {
			file_block_entry_type* entry = m_blocks.rbegin()->second;
			entry->refresh_if_needed();
			return entry->block().end_time();
		}
	}


	void register_at_monitor(generic_param_entry* entry, SzbParamMonitor* monitor) {
		monitor->add_observer(entry, m_param, m_param_dir.external_file_string(), 0);
	}

	void deregister_from_monitor(generic_param_entry* entry, SzbParamMonitor* monitor) {
		monitor->remove_observer(entry);
	}

	void param_data_changed(TParam*, const std::string& path) {
		boost::mutex::scoped_lock lock(m_mutex);
		m_paths_to_update.push_back(path);
	}

	~real_param_entry_in_buffer() {
		while (m_blocks.size())
			delete m_blocks.begin()->second;
	}
};

}
#endif
