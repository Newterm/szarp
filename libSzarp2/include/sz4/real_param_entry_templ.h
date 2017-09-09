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
#ifndef __SZ4_REAL_PARAM_ENTRY_TEMPL_H__
#define __SZ4_REAL_PARAM_ENTRY_TEMPL_H__

#include "config.h"

#include <fstream>

#include <boost/container/flat_map.hpp>

#if BOOST_FILESYSTEM_VERSION != 3
#define filesystem_error wfilesystem_error
#define directory_iterator wdirectory_iterator
#endif

#include "decode_file.h"
#include "live_cache.h"
#include "load_current_file.h"

namespace sz4 {

template<class value_type, class time_type, class base>
file_block<value_type, time_type, base>::file_block(const time_type& start_time
	, entry_type* entry
	, block_cache* cache)
	: concrete_block<value_type, time_type>(start_time, cache)
	, m_entry(entry)
{}

template<class value_type, class time_type, class base>
file_block<value_type, time_type, base>::~file_block() {
	m_entry->block_deleted();
}

template<class V, class T, class base> file_block_entry<V, T, base>::file_block_entry(const T& start_time
	, const std::wstring& block_path
	, block_cache* cache) 
	: m_block(nullptr)
	, m_start_time(start_time)
	, m_cache(cache)
	, m_needs_refresh(true)
	, m_block_path(block_path)
{
}

template<class V, class T, class base> T file_block_entry<V, T, base>::start_time() {
	return m_start_time;
}

template<class V, class T, class base> T file_block_entry<V, T, base>::end_time() {
	refresh_if_needed();
	return m_block->end_time();
}

template<class V, class T, class base>
void file_block_entry<V, T, base>::get_weighted_sum(const T& start, const T& end, weighted_sum<V, T>& wsum) {
	refresh_if_needed();
	m_block->get_weighted_sum(start, end, wsum);
}

template<class V, class T, class base>
T file_block_entry<V, T, base>::search_data_right(const T& start, const T& end, const search_condition& condition) {
	refresh_if_needed();
	return m_block->search_data_right(start, end, condition);
}

template<class V, class T, class base>
T file_block_entry<V, T, base>::search_data_left(const T& start, const T& end, const search_condition& condition) {
	refresh_if_needed();
	return m_block->search_data_left(start, end, condition);
}

template<class V, class T, class base>
void file_block_entry<V, T, base>::set_needs_refresh() {
	m_needs_refresh = true;
}

template<class V, class T, class base>
void file_block_entry<V, T, base>::block_deleted() {
	m_block = NULL;
}

template<class V, class T, class base>
file_block_entry<V, T, base>::~file_block_entry() {
	delete m_block;
}


template<class V, class T, class base>
sz4_file_block_entry<V, T, base>::sz4_file_block_entry(const T& start_time
	, const std::wstring& block_path
	, block_cache* cache)
	: parent(time_trait<T>::invalid_value, block_path, cache)
{}

template<class V, class T, class base>
void sz4_file_block_entry<V, T, base>::refresh_if_needed() {
	if (!this->m_block) {
		this->m_block = new typename parent::block_type(this->m_start_time, this, this->m_cache);
		this->m_needs_refresh = true;
	}

	if (!this->m_needs_refresh)
		return;

	std::vector<unsigned char> buffer;
	if (load_bz2_file(this->m_block_path, buffer)) {
		std::vector<value_time_pair<V, T> > values = decode_file<V, T>(&buffer[0], buffer.size(), this->start_time());
		this->m_block->set_data(values);
	}

	this->m_needs_refresh = false;
}

template<class V, class T, class block_map, class base>
sz4_current_block_entry<V, T, block_map, base>::sz4_current_block_entry(block_map* map
	, const std::wstring& block_path
	, block_cache* cache)
	: parent(time_trait<T>::last_valid_time, block_path, cache)
	, m_map(map)
{
	this->m_start_time = time_trait<T>::invalid_value;
}

template<class V, class T, class block_map, class base>
void sz4_current_block_entry<V, T, block_map, base>::refresh_if_needed() {
	if (!this->m_block) {
		this->m_block = new typename parent::block_type(time_trait<T>::last_valid_time, this, this->m_cache);
		this->m_needs_refresh = true;
	}

	if (!this->m_needs_refresh)
		return;
	this->m_needs_refresh = false;

	T last_time;
	if (m_map->size() == 0)
		last_time = time_trait<T>::invalid_value;
	else
		last_time = m_map->rbegin()->second->end_time();

	std::vector<unsigned char> buffer;
	T start_time;
	if (!load_current_file(this->m_block_path, start_time, buffer))
		return;

	if (!time_trait<T>::is_valid(this->m_start_time))
		this->m_start_time = start_time;

	//transient state
	if (start_time != this->m_start_time && last_time != start_time)
		return;

	this->m_start_time = start_time;
	std::vector<value_time_pair<V, T> > values = decode_file<V, T>(&buffer[0], buffer.size(), start_time);
	this->m_block->set_data(values);
}

template<class V, class T, class base>
szbase_file_block_entry<V, T, base>::szbase_file_block_entry(const T& start_time
	, const std::wstring& block_path
	, block_cache* cache)
	: parent(start_time, block_path, cache)
	, m_read_so_far(0)
{}

template<class V, class T, class base>
void szbase_file_block_entry<V, T, base>::refresh_if_needed() {
	if (!this->m_block) {
		this->m_block = new typename parent::block_type(this->m_start_time, this, this->m_cache);
		this->m_needs_refresh = true;
		this->m_read_so_far = 0;
	}

	if (!this->m_needs_refresh)
		return;

	boost::system::error_code ec;
	size_t size = boost::filesystem::file_size(this->m_block_path, ec);
	if (ec)
		return;

	size_t shorts_to_read = (size > this->m_read_so_far)
					? (size - this->m_read_so_far) / 2 : 0;

	if (!shorts_to_read) {
		this->m_needs_refresh = false;
		return;
	}

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

	this->m_needs_refresh = false;

	T t = szb_move_time(this->end_time(), 1, PT_MIN10);
	for (size_t i = 0; i < buffer.size(); i++, t = szb_move_time(t, 1, PT_MIN10))
		this->m_block->append_entry(buffer[i], t);

	this->m_read_so_far += 2 * shorts_to_read;
}


template<class V, class T, class base>
real_param_entry_in_buffer<V, T, base>::real_param_entry_in_buffer(base *_base
	, TParam* param
	, const boost::filesystem::wpath& param_dir)
	: m_base(_base)
	, m_param(param)
	, m_param_dir(param_dir)
	, m_refresh_file_list(true)
	, m_has_paths_to_update(false)
	, m_first_sz4_date(time_trait<T>::invalid_value)
	, m_live_block(nullptr)
	, m_current_block(nullptr)
{}

template<class V, class T, class base>
void real_param_entry_in_buffer<V, T, base>::get_weighted_sum_impl(const T& start, const T& _end, SZARP_PROBE_TYPE, weighted_sum<V, T>& sum) {

	T end(_end);
	cache_ret from_live = cache_ret::none;
	if (m_live_block) {
		from_live = m_live_block->get_weighted_sum(start, end, sum);
		if (from_live == cache_ret::complete)
			return;
	}

	refresh_if_needed();

	file_block_entry_type* entry(nullptr);
	T current(start);
	typename map_type::iterator i = m_blocks.upper_bound(start);
	//if not first or end - back one off
	if (i != m_blocks.begin() && i != m_blocks.end())
		std::advance(i, -1);

	while (true) {
		if (i == m_blocks.end()) {
			if (entry != m_current_block)
				entry = m_current_block;
			else
				break;
		} else
			entry = i->second;

		if (!entry)
			break;

		if (end < entry->start_time()) {
			sum.add_no_data_weight(end - current);
			return;
		}

		if (current < entry->start_time()) {
			sum.add_no_data_weight(entry->start_time() - current);
			current = i->first;
		}

		if (current < entry->end_time()) {
			T end_for_block = std::min(end, entry->end_time());
			entry->get_weighted_sum(current, end_for_block, sum);

			current = end_for_block;
		}

		if (i != m_blocks.end())
			std::advance(i, 1);
	}

	if (current < end) {
		sum.add_no_data_weight(end - current);
		if (from_live == cache_ret::none)
			sum.set_fixed(false);
	}
}

template<class V, class T, class base>
T real_param_entry_in_buffer<V, T, base>::search_data_right_impl(const T& start, const T& end, SZARP_PROBE_TYPE, const search_condition& condition) {
	refresh_if_needed();

	if (m_blocks.size()) {
		typename map_type::iterator i = m_blocks.upper_bound(start);
		//if not first - step one back
		if (i != m_blocks.begin())
			std::advance(i, -1);

		while (i != m_blocks.end()) {
			file_block_entry_type* entry = i->second;
			if (entry->start_time() >= end)
				break;

			T t = entry->search_data_right(start, end, condition);
			if (time_trait<T>::is_valid(t))
				return t;

			std::advance(i, 1);
		}
	}

	if (m_current_block) {
		T t = m_current_block->search_data_right(start, end, condition);
		if (time_trait<T>::is_valid(t))
			return t;
	}

	if (m_live_block)
		return m_live_block->search_data_right(start, end, condition);
	else
		return time_trait<T>::invalid_value;
}

template<class V, class T, class base>
T real_param_entry_in_buffer<V, T, base>::search_data_left_impl(const T& _start, const T& end, SZARP_PROBE_TYPE, const search_condition& condition) {
	T start(_start);

	if (m_live_block) {
		auto result = m_live_block->search_data_left(start, end, condition);

		if (result.first)
			return result.second;

		start = result.second;
	}

	refresh_if_needed();

	if (m_current_block && end > m_current_block->start_time()) {
		T t = m_current_block->search_data_left(start, end, condition);
		if (time_trait<T>::is_valid(t))
			return t;
	}

	if (m_blocks.size() == 0)
		return time_trait<T>::invalid_value;

	typename map_type::iterator i = m_blocks.upper_bound(start);
	if (i != m_blocks.begin())
		std::advance(i, -1);
	else if (i->first > start)
		return time_trait<T>::invalid_value;

	while (true) {
		file_block_entry_type* entry = i->second;

		if (entry->end_time() <= end)
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

template<class V, class T, class base>
void real_param_entry_in_buffer<V, T, base>::refresh_if_needed() {
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

template<class V, class T, class base>
void real_param_entry_in_buffer<V, T, base>::refresh_file(const std::string& path) {
	bool sz4_file;
	T time = path_to_date<T>(path, sz4_file);
	if (!time_trait<T>::is_valid(time))
		return;

	if (time == time_trait<T>::last_valid_time) {
		if (m_current_block)
			m_current_block->set_needs_refresh();	
		else
			m_refresh_file_list = true;
	} else {
		typename map_type::iterator i = m_blocks.find(time);
		if (i != m_blocks.end())
			i->second->set_needs_refresh();
		else {
			if (sz4_file == true || !time_trait<T>::is_valid(m_first_sz4_date))
				m_refresh_file_list = true;
		}
	}
}

template<class V, class T, class base>
void real_param_entry_in_buffer<V, T, base>::refresh_file_list() {
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

		if (sz4 && (!time_trait<T>::is_valid(m_first_sz4_date) || m_first_sz4_date > file_time))
			m_first_sz4_date = file_time;
		
		new_files.push_back(file_entry(file_time, file_path, sz4));
	}


	std::sort(new_files.begin(), new_files.end(),
		[] (const file_entry& _1, const file_entry& _2) { return std::get<0>(_1) < std::get<0>(_2); });

	for (auto& f : new_files) {
		auto& file_time = std::get<0>(f);
		auto& file_path = std::get<1>(f);
		bool sz4 = std::get<2>(f);

		file_block_entry<V, T, base> *entry(nullptr);
		bool current(false);
		if (sz4) {
			if (time_trait<T>::last_valid_time == file_time) {
				current = true;
				entry = new sz4_current_block_entry<V, T, map_type, base>(
						&m_blocks, file_path, m_base->cache());
			} else
				entry = new sz4_file_block_entry<V, T, base>(
						file_time, file_path, m_base->cache());
		} else {
			if (!time_trait<T>::is_valid(m_first_sz4_date) || m_first_sz4_date > file_time)
				entry = new szbase_file_block_entry<V, T, base>(
						file_time, file_path, m_base->cache());
		}

		if (current)
			m_current_block = entry;
		else
			m_blocks.insert(std::make_pair(file_time, entry));

	}
}

template<class V, class T, class base>
void real_param_entry_in_buffer<V, T, base>::get_first_time(const std::list<generic_param_entry*>&, T &t) {
	if (m_live_block) {
		m_live_block->get_first_time(t);
		if (time_trait<T>::is_valid(t))
			return;
	}

	refresh_if_needed();

	if (m_blocks.size() == 0)
		t = time_trait<T>::invalid_value;
	else
		t = m_blocks.begin()->first;
}

template<class V, class T, class base>
void  real_param_entry_in_buffer<V, T, base>::get_last_time(const std::list<generic_param_entry*>&, T &t) {
	if (m_live_block) {
		m_live_block->get_last_time(t);
		if (time_trait<T>::is_valid(t))
			return;
	}

	refresh_if_needed();
	t = time_trait<T>::invalid_value;
	for (typename map_type::reverse_iterator i = m_blocks.rbegin();
			!time_trait<T>::is_valid(t) && i != m_blocks.rend(); i++) {
		file_block_entry_type* entry = i->second;
		entry->refresh_if_needed();
		t = entry->end_time();
	}
}

template<class V, class T, class base>
void real_param_entry_in_buffer<V, T, base>::register_at_monitor(generic_param_entry* entry, SzbParamMonitor* monitor) {
#if BOOST_FILESYSTEM_VERSION == 3
	monitor->add_observer(entry, m_param, m_param_dir.string(), 0);
#else
	monitor->add_observer(entry, m_param, m_param_dir.external_file_string(), 0);
#endif
}

template<class V, class T, class base>
void real_param_entry_in_buffer<V, T, base>::deregister_from_monitor(generic_param_entry* entry, SzbParamMonitor* monitor) {
	monitor->remove_observer(entry);
}

template<class V, class T, class base>
void real_param_entry_in_buffer<V, T, base>::param_data_changed(TParam*, const std::string& path) {
	boost::mutex::scoped_lock lock(m_mutex);
	m_paths_to_update.push_back(path);
	m_has_paths_to_update = true;
}

template<class V, class T, class base>
void real_param_entry_in_buffer<V, T, base>::set_live_block(generic_live_block* block) {
	m_live_block = dynamic_cast<live_block<V, T>*>(block);	
	assert(block && m_live_block);
}

template<class V, class T, class base>
real_param_entry_in_buffer<V, T, base>::~real_param_entry_in_buffer() {
	for (auto kv : m_blocks)
		delete kv.second;
}

}

#if BOOST_FILESYSTEM_VERSION != 3
#undef filesystem_error
#undef directory_iterator
#endif

#endif
