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
#include <boost/thread/recursive_mutex.hpp>

#include "szarp_base_common/szbparamobserver.h"
#include "szarp_base_common/szbparammonitor.h"

#include "szarp_base_common/defines.h"
#include "sz4/defs.h"
#include "sz4/types.h"
#include "sz4/time.h"
#include "sz4/block.h"
#include "sz4/path.h"
#include "sz4/param_observer.h"
#include "sz4/load_file_locked.h"
#include "sz4/param_entry.h"
#include "sz4/param_entry_factory.h"

namespace sz4 {

template<class types> class base_templ;

template<class types> class buffer_templ {
	typedef typename types::ipk_container_type ipk_container_type;
	base_templ<types>* m_base;
	SzbParamMonitor* m_param_monitor;
	ipk_container_type* m_ipk_container;
	boost::filesystem::wpath m_buffer_directory;
	std::vector<generic_param_entry*> m_param_ents;

	generic_param_entry* m_heart_beat_entry;

	void prepare_param(TParam* param);
public:
	buffer_templ(base_templ<types>* _base, SzbParamMonitor* param_monitor, ipk_container_type* ipk_container, const std::wstring& prefix, const std::wstring& buffer_directory);

	generic_param_entry* get_param_entry(TParam* param);

	template<class T, class V> void get_weighted_sum(TParam* param, const T& start, const T &end, SZARP_PROBE_TYPE probe_type, weighted_sum<V, T> &wsum) {
		get_param_entry(param)->get_weighted_sum(start, end, probe_type, wsum);
	}

	template<class T> T search_data_right(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition &condition) {
		return get_param_entry(param)->search_data_right(start, end, probe_type, condition);
	}

	template<class T> T search_data_left(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition &condition) {
		return get_param_entry(param)->search_data_left(start, end, probe_type, condition);
	}

	generic_param_entry* create_param_entry(TParam* param);

	void get_heartbeat_first_time(nanosecond_time_t& t) {
		m_heart_beat_entry->get_first_time(t);
	}

	void get_heartbeat_first_time(second_time_t& t) {
		m_heart_beat_entry->get_first_time(t);
	}

	void get_first_time(TParam* param, nanosecond_time_t& t) {
		get_param_entry(param)->get_first_time(t);
	}

	void get_first_time(TParam* param, second_time_t& t) {
		get_param_entry(param)->get_first_time(t);
	}

	void get_heartbeat_last_time(nanosecond_time_t& t) {
		m_heart_beat_entry->get_last_time(t);
	}

	void get_heartbeat_last_time(second_time_t& t) {
		m_heart_beat_entry->get_last_time(t);
	}

	void get_heartbeat_last_time(TParam* param, nanosecond_time_t& t) {
		get_param_entry(param)->get_last_time(t);
	}

	void get_last_time(TParam* param, second_time_t& t) {
		get_param_entry(param)->get_last_time(t);
	}

	void get_last_time(TParam* param, nanosecond_time_t& t) {
		get_param_entry(param)->get_last_time(t);
	}

	void remove_param(TParam* param);

	~buffer_templ();
};

typedef buffer_templ<base_types> buffer;

}

#endif
