#ifndef __SZ4_BASE_H__
#define __SZ4_BASE_H__
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
#include "config.h"

#include <vector>
#include <stack>
#include <set>

#include <lua.hpp>

#include <boost/filesystem/path.hpp>

#include "sz4/defs.h"
#include "sz4/types.h"
#include "sz4/buffer.h"
#include "sz4/lua_interpreter.h"
#include "sz4/block_cache.h"
#include "sz4/param_observer.h"
#include "szarp_base_common/lua_strings_extract.h"

namespace sz4 {

template<class types> class buffer_templ;

template<class types> class base_templ {
public:
	typedef typename types::ipk_container_type ipk_container_type;
private:
	const boost::filesystem::wpath m_szarp_data_dir;
	std::vector<buffer_templ<types>*> m_buffers;
	SzbParamMonitor m_monitor;
	ipk_container_type* m_ipk_container;

	std::stack<std::pair<bool, std::set<generic_block*> > > m_fixed_stack;
	lua_interpreter<types> m_interperter;

	block_cache m_cache;
public:
	base_templ(const std::wstring& szarp_data_dir, ipk_container_type* ipk_container) : m_szarp_data_dir(szarp_data_dir), m_ipk_container(ipk_container) {
		m_interperter.initialize(this, m_ipk_container);
	}

	template<class V, class T> void get_weighted_sum(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, weighted_sum<V, T>& sum) {
		buffer_for_param(param)->get_weighted_sum(param, start, end, probe_type, sum);
	}

	template<class T> T search_data_right(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return buffer_for_param(param)->search_data_right(param, start, end, probe_type, condition);
	}

	template<class T> T search_data_left(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return buffer_for_param(param)->search_data_left(param, start, end, probe_type, condition);
	}

	template<class T> void get_first_time(TParam* param, T& t) {
		buffer_for_param(param)->get_first_time(param, t);
	}

	template<class T> void get_heartbeat_first_time(TParam* param, T& t) {
		buffer_for_param(param)->get_heartbeat_first_time(t);
	}

	template<class T> void get_last_time(TParam* param, T& t) {
		buffer_for_param(param)->get_last_time(param, t);
	}

	template<class T> void get_heartbeat_last_time(TParam* param, T& t) {
		buffer_for_param(param)->get_heartbeat_last_time(t);
	}


	buffer_templ<types>* buffer_for_param(TParam* param) {
		buffer_templ<types>* buf;
		if (param->GetConfigId() >= m_buffers.size())
			m_buffers.resize(param->GetConfigId() + 1, NULL);
		buf = m_buffers[param->GetConfigId()];

		if (buf == NULL) {
			std::wstring prefix = param->GetSzarpConfig()->GetPrefix();	
#if BOOST_FILESYSTEM_VERSION == 3
			buf = m_buffers[param->GetConfigId()] = new buffer_templ<types>(this, &m_monitor, m_ipk_container, prefix, (m_szarp_data_dir / prefix / L"sz4").wstring());
#else
			buf = m_buffers[param->GetConfigId()] = new buffer_templ<types>(this, &m_monitor, m_ipk_container, prefix, (m_szarp_data_dir / prefix / L"sz4").file_string());
#endif
		}
		return buf;
	}

	generic_param_entry* get_param_entry(TParam* param) {
		return buffer_for_param(param)->get_param_entry(param);
	}

	void remove_param(TParam* param) {
		if (param->GetConfigId() >= m_buffers.size())
			return;

		buffer_templ<types>* buf = m_buffers[param->GetConfigId()];
		if (buf == NULL)
			return;

		buf->remove_param(param);
	}

	void register_observer(param_observer *observer, const std::vector<TParam*>& params) {
		for (std::vector<TParam*>::const_iterator i = params.begin(); i != params.end(); i++) {
			generic_param_entry *entry = get_param_entry(*i);
			if (entry)
				entry->register_observer(observer);
		}
	}

	void deregister_observer(param_observer *observer, const std::vector<TParam*>& params) {
		for (std::vector<TParam*>::const_iterator i = params.begin(); i != params.end(); i++) {
			generic_param_entry *entry = get_param_entry(*i);
			if (entry)
				entry->deregister_observer(observer);
		}
	}

	std::stack<std::pair<bool, std::set<generic_block*> > >& fixed_stack() { return m_fixed_stack; }

	SzbParamMonitor& param_monitor() { return m_monitor; }

	lua_interpreter<types>& get_lua_interpreter() { return m_interperter; }

	ipk_container_type* get_ipk_container() { return m_ipk_container; }

	block_cache* cache() { return &m_cache; }

	~base_templ() {
		for (typename std::vector<buffer_templ<types>*>::iterator i = m_buffers.begin(); i != m_buffers.end(); i++)
			delete *i;
	}

};

typedef base_templ<base_types> base;

}

#endif
