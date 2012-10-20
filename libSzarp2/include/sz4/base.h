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
#ifndef __SZ4_BASE_H__
#define __SZ4_BASE_H__

#include "config.h"

#include <vector>
#include <stack>

#include <boost/filesystem/path.hpp>

#include "sz4/defs.h"
#include "sz4/buffer.h"

class IPKContainer;

namespace sz4 {

template<class ipk_container_type> class base_templ {
	const boost::filesystem::wpath m_szarp_data_dir;
	std::vector<buffer_templ<ipk_container_type>*> m_buffers;
	SzbParamMonitor m_monitor;
	ipk_container_type* m_ipk_container;

	std::stack<bool> m_fixed_stack;
public:
	base_templ(const std::wstring& szarp_data_dir, ipk_container_type* ipk_container) : m_szarp_data_dir(szarp_data_dir), m_ipk_container(ipk_container) {}

	template<class V, class T> void get_weighted_sum(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, weighted_sum<V, T>& sum) {
		buffer_for_param(param)->get_weighted_sum(param, start, end, probe_type, sum);
	}

	template<class T> T search_data_right(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return buffer_for_param(param)->search_data_right(param, start, end, probe_type, condition);
	}

	template<class T> T search_data_left(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition) {
		return buffer_for_param(param)->search_data_left(param, start, end, probe_type, condition);
	}

	buffer_templ<ipk_container_type>* buffer_for_param(TParam* param) {
		buffer_templ<ipk_container_type>* buf;
		if (param->GetConfigId() >= m_buffers.size())
			m_buffers.resize(param->GetConfigId() + 1, NULL);
		buf = m_buffers[param->GetConfigId()];

		if (buf == NULL)
			buf = m_buffers[param->GetConfigId()] = new buffer_templ<ipk_container_type>(this, &m_monitor, m_ipk_container, (m_szarp_data_dir / param->GetSzarpConfig()->GetPrefix() / L"sz4").file_string());

		return buf;
	}

	generic_param_entry* get_param_entry(TParam* param) {
		return buffer_for_param(param)->get_param_entry(param);
	}

	void remove_param(TParam* param) {
		if (param->GetConfigId() >= m_buffers.size())
			return;

		buffer* buf = m_buffers[param->GetConfigId()];
		if (buf == NULL)
			return;

		buf->remove_param(param);
	}

	std::stack<bool>& fixed_stack() { return m_fixed_stack; }

	SzbParamMonitor& param_monitor() { return m_monitor; }

	~base_templ() {
		for (typename std::vector<buffer_templ<ipk_container_type>*>::iterator i = m_buffers.begin(); i != m_buffers.end(); i++)
			delete *i;
	}

};

typedef base_templ<IPKContainer> base;

}

#endif
