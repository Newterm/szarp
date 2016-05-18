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
#include <set>

#include <lua.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

#include "sz4/types.h"
#include "sz4/block.h"
#include "sz4/block_cache.h"
#include "sz4/buffer.h"
#include "sz4/lua_interpreter.h"
#include "sz4/defs.h"
#include "sz4/param_observer.h"
#include "sz4/param_observer.h"
#include "szarp_base_common/lua_strings_extract.h"

namespace sz4 {

class live_cache;
template<class types> class buffer_templ;

typedef std::vector<
		bool
	> fixed_stack_type;

template<class types> class base_templ {
public:
	typedef typename types::ipk_container_type ipk_container_type;
private:
	const boost::filesystem::wpath m_szarp_data_dir;
	std::vector<buffer_templ<types>*> m_buffers;
	SzbParamMonitor m_monitor;
	ipk_container_type* m_ipk_container;

	fixed_stack_type m_fixed_stack;
	lua_interpreter<types> m_interperter;

	block_cache m_cache;

	std::unique_ptr<live_cache> m_live_cache;

	boost::optional<SZARP_PROBE_TYPE> m_read_ahead;
public:
	base_templ(const std::wstring& szarp_data_dir, ipk_container_type* ipk_container);

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

	boost::optional<SZARP_PROBE_TYPE>& read_ahead();

	buffer_templ<types>* buffer_for_param(TParam* param);

	generic_param_entry* get_param_entry(TParam* param);

	void remove_param(TParam* param);

	void register_observer(param_observer *observer, const std::vector<TParam*>& params);

	void deregister_observer(param_observer *observer, const std::vector<TParam*>& params);

	fixed_stack_type& fixed_stack();

	SzbParamMonitor& param_monitor();

	lua_interpreter<types>& get_lua_interpreter();

	ipk_container_type* get_ipk_container();

	block_cache* cache();

	live_cache* get_live_cache();

	~base_templ();
};

typedef base_templ<base_types> base;

}

#endif
