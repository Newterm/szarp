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

#if BOOST_FILESYSTEM_VERSION != 3
#define filesystem_error wfilesystem_error
#define directory_iterator wdirectory_iterator
#endif

#include <fstream>

#include "decode_file.h"

namespace sz4 {

template<class V, class T, class types> class real_param_entry_in_buffer;

template<class V, class T, class types> class file_block_entry : public block_entry<V, T> {
protected:
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

	virtual void refresh_if_needed() = 0;

	virtual ~file_block_entry() {
		m_param_entry->remove_block(this);
	}
};

template<class V, class T, class types> class sz4_file_block_entry : public file_block_entry<V, T, types> {
public:
	typedef file_block_entry<V, T, types> parent;
	sz4_file_block_entry(const T& start_time,
			const std::wstring& block_path,
			block_cache* cache,
			real_param_entry_in_buffer<V, T, types>* param_entry)
		:
			parent(start_time, block_path, cache, param_entry)
		{}

	void refresh_if_needed() {
		if (!this->m_needs_refresh)
			return;

		size_t size = boost::filesystem::file_size(this->m_block_path);
		std::vector<unsigned char> buffer(size);

		if (load_file_locked(this->m_block_path, &buffer[0], buffer.size())) {
			std::vector<value_time_pair<V, T> > values = decode_file<V, T>(&buffer[0], buffer.size(), this->m_block.start_time());
			this->m_block.set_data(values);
			this->m_needs_refresh = false;
		}
	}
};

template<class V, class T, class types> class szbase_file_block_entry : public file_block_entry<V, T, types> {
	size_t m_read_so_far;
public:
	typedef file_block_entry<V, T, types> parent;

	szbase_file_block_entry(const T& start_time,
			const std::wstring& block_path,
			block_cache* cache,
			real_param_entry_in_buffer<V, T, types>* param_entry)
		:
			parent(start_time, block_path, cache, param_entry),
			m_read_so_far(0)
		{}

	void refresh_if_needed() {
		if (!this->m_needs_refresh)
			return;

		size_t size = boost::filesystem::file_size(this->m_block_path);
		size_t shorts_to_read = (size > this->m_read_so_far)
						? (size - this->m_read_so_far) / 2 : 0;

		if (shorts_to_read) {
			std::vector<short> buffer(shorts_to_read);
			std::ifstream ifs(
#if BOOST_FILESYSTEM_VERSION == 3
					this->m_block_path.string().c_str(),
#else
					this->m_block.path.external_file_string().c_str(),
#endif
					std::ios::binary | std::ios::in);

			if (!ifs.seekg(m_read_so_far, std::ios_base::beg))
				return;

			if (!ifs.read((char*)&buffer[0], 2 * shorts_to_read))
				return;

			T t = szb_move_time(this->block().end_time(), 1, PT_MIN10);
			for (size_t i = 0; i < buffer.size(); i++, t = szb_move_time(t, 1, PT_MIN10))
				this->block().append_entry(buffer[i], t);

			this->m_read_so_far += 2 * shorts_to_read;
		}

		this->m_needs_refresh = false;
	}
};


template<class V, class T, class types> class real_param_entry_in_buffer {
public:
	typedef file_block_entry<V, T, types> file_block_entry_type;
	typedef boost::container::flat_map<T, file_block_entry_type*, std::less<T>, allocator_type<std::pair<T, file_block_entry_type*> > > map_type;
	//typedef std::map<T, file_block_entry_type*, std::less<T>, allocator_type<std::pair<const T, file_block_entry_type*> > > map_type;
private:
	base_templ<types>* m_base;
	map_type m_blocks;

	TParam* m_param;
	const boost::filesystem::wpath m_param_dir;
	std::vector<std::string> m_paths_to_update;

	boost::mutex m_mutex;

	bool m_refresh_file_list;
	bool m_has_paths_to_update;

	T m_first_sz4_date;
public:
	real_param_entry_in_buffer(base_templ<types> *_base, TParam* param, const boost::filesystem::wpath& param_dir) : m_base(_base), m_param(param), m_param_dir(param_dir), m_refresh_file_list(true), m_has_paths_to_update(false), m_first_sz4_date(time_trait<T>::invalid_value)
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
			return time_trait<T>::invalid_value;

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
			if (time_trait<T>::is_valid(t))
				return t;

			std::advance(i, 1);
		}

		return time_trait<T>::invalid_value;
	}

	T search_data_left_impl(const T& start, const T& end, SZARP_PROBE_TYPE, const search_condition& condition) {
		refresh_if_needed();
		if (m_blocks.size() == 0)
			return time_trait<T>::invalid_value;

		typename map_type::iterator i = m_blocks.upper_bound(start);
		if (i != m_blocks.begin())
			std::advance(i, -1);
		else if (i->first > start)
			return time_trait<T>::invalid_value;

		while (true) {
			file_block_entry_type* entry = i->second;

			entry->refresh_if_needed();
			if (entry->block().end_time() <= end)
				break;

			T t = entry->search_data_left(start, end, condition);
			if (time_trait<T>::is_valid(t))
				return t;

			if (i == m_blocks.begin())
				break;
			std::advance(i, -1);
		}

		return time_trait<T>::invalid_value;
	}

	void refresh_if_needed() {
		if (m_has_paths_to_update) {
			boost::mutex::scoped_lock lock(m_mutex);
			for (std::vector<std::string>::iterator i = m_paths_to_update.begin(); i != m_paths_to_update.end(); i++) {
				refresh_file(*i);
			}
			m_paths_to_update.clear();
			m_has_paths_to_update = false;
		}
		if (m_refresh_file_list) {
			try {
				refresh_file_list();
			} catch (boost::filesystem::filesystem_error&) {}
			m_refresh_file_list = false;
		}
	}

	
	void refresh_file(const std::string& path) {
		bool sz4_file;
		T time = path_to_date<T>(path, sz4_file);
		if (!time_trait<T>::is_valid(time))
			return;

		typename map_type::iterator i = m_blocks.find(time);
		if (i != m_blocks.end())
			i->second->set_needs_refresh();
		else {
			if (sz4_file == true || !time_trait<T>::is_valid(m_first_sz4_date))
				m_refresh_file_list = true;
		}
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

		typedef std::tuple<T, std::wstring, bool> file_entry;
		std::vector<file_entry> new_files;
		
		for (fs::directory_iterator i(m_param_dir);
				i != fs::directory_iterator();
				i++) {

#if BOOST_FILESYSTEM_VERSION == 3
			std::wstring file_path = i->path().wstring();
#else
			std::wstring file_path = i->path().file_string();
#endif

			bool sz4;
			T file_time = path_to_date<T>(file_path, sz4);
			if (!time_trait<T>::is_valid(file_time))
				continue;

			if (m_blocks.find(file_time) != m_blocks.end())
				continue;
			
			new_files.push_back(file_entry(file_time, file_path, sz4));
		}


		std::sort(new_files.begin(), new_files.end(),
			[] (const file_entry& _1, const file_entry& _2) { return std::get<0>(_1) < std::get<0>(_2); });

		for (auto& f : new_files) {
			auto& file_time = std::get<0>(f);
			auto& file_path = std::get<1>(f);
			bool sz4 = std::get<2>(f);

			file_block_entry<V, T, types> *entry(nullptr);
			if (sz4) {
				entry = new sz4_file_block_entry<V, T, types>(
							file_time, file_path, m_base->cache(), this);
				if (time_trait<T>::is_valid(m_first_sz4_date) || m_first_sz4_date > file_time)
					m_first_sz4_date = file_time;
			} else {
				if (!time_trait<T>::is_valid(m_first_sz4_date) || m_first_sz4_date > file_time)
					entry = new szbase_file_block_entry<V, T, types>(
							file_time, file_path, m_base->cache(), this);
			}

			if (entry)
				m_blocks.insert(std::make_pair(file_time, entry));

		}
	}

	void get_first_time(const std::list<generic_param_entry*>&, T &t) {
		refresh_if_needed();
		if (m_blocks.size() == 0)
			t = time_trait<T>::invalid_value;
		else
			t = m_blocks.begin()->first;
	}

	void  get_last_time(const std::list<generic_param_entry*>&, T &t) {
		refresh_if_needed();
		t = time_trait<T>::invalid_value;
		for (typename map_type::reverse_iterator i = m_blocks.rbegin();
				!time_trait<T>::is_valid(t) && i != m_blocks.rend(); i++) {
			file_block_entry_type* entry = i->second;
			entry->refresh_if_needed();
			t = entry->block().end_time();
		}
	}

	void register_at_monitor(generic_param_entry* entry, SzbParamMonitor* monitor) {
#if BOOST_FILESYSTEM_VERSION == 3
		monitor->add_observer(entry, m_param, m_param_dir.string(), 0);
#else
		monitor->add_observer(entry, m_param, m_param_dir.external_file_string(), 0);
#endif
	}

	void deregister_from_monitor(generic_param_entry* entry, SzbParamMonitor* monitor) {
		monitor->remove_observer(entry);
	}

	void param_data_changed(TParam*, const std::string& path) {
		boost::mutex::scoped_lock lock(m_mutex);
		m_paths_to_update.push_back(path);
		m_has_paths_to_update = true;
	}

	~real_param_entry_in_buffer() {
		while (m_blocks.size()) {
			file_block_entry_type* entry = m_blocks.rbegin()->second;
			delete entry;
		}
	}
};

}

#if BOOST_FILESYSTEM_VERSION != 3
#undef filesystem_error
#undef directory_iterator
#endif

#endif
