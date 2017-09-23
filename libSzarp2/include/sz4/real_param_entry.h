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

template<class V, class T> class live_block;

template<class V, class T, class base> class file_block_entry;

template<class value_type, class time_type, class base>
class file_block : public concrete_block<value_type, time_type> {
public:
	typedef file_block_entry<value_type, time_type, base> entry_type;
	file_block(const time_type& start_time,
			entry_type* entry,
			block_cache* cache);

	~file_block();

protected:
	entry_type *m_entry;
};

template<class V, class T, class base> class real_param_entry_in_buffer {
public:
	typedef file_block_entry<V, T, base> file_block_entry_type;
	typedef boost::container::flat_map<T, file_block_entry_type*, std::less<T>, allocator_type<std::pair<T, file_block_entry_type*> > > map_type;
protected:
	base* m_base;
	map_type m_blocks;

	TParam* m_param;
	const boost::filesystem::wpath m_param_dir;
	std::vector<std::string> m_paths_to_update;

	boost::mutex m_mutex;

	bool m_refresh_file_list;
	bool m_has_paths_to_update;

	T m_first_sz4_date;

	live_block<V, T>* m_live_block;

	file_block_entry<V, T, base>* m_current_block;
public:
	real_param_entry_in_buffer(base *_base, TParam* param, const boost::filesystem::wpath& param_dir);

	void get_weighted_sum_impl(const T& start, const T& end, SZARP_PROBE_TYPE, weighted_sum<V, T>& sum);

	T search_data_right_impl(const T& start, const T& end, SZARP_PROBE_TYPE, const search_condition& condition);

	T search_data_left_impl(const T& start, const T& end, SZARP_PROBE_TYPE, const search_condition& condition);

	void refresh_if_needed();
	
	void refresh_file(const std::string& path);

	void refresh_file_list();

	void get_first_time(const std::list<generic_param_entry*>&, T &t);

	void get_last_time(const std::list<generic_param_entry*>&, T &t);

	void register_at_monitor(generic_param_entry* entry, SzbParamMonitor* monitor);

	void deregister_from_monitor(generic_param_entry* entry, SzbParamMonitor* monitor);

	void param_data_changed(TParam*, const std::string& path);

	void set_live_block(generic_live_block *block);

	~real_param_entry_in_buffer();
};

}

#endif
