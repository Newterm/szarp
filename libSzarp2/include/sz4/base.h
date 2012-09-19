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

#include <boost/filesystem/path.hpp>

#include "sz4/defs.h"
#include "sz4/buffer.h"

namespace sz4 {

class base {
	const boost::filesystem::wpath m_szarp_data_dir;
	std::vector<buffer*> m_buffers;
	SzbParamMonitor m_monitor;

	buffer* buffer_for_param(TParam* param) {
		buffer* buf;
		if (param->GetConfigId() >= m_buffers.size())
			m_buffers.resize(param->GetConfigId() + 1, NULL);
		buf = m_buffers[param->GetConfigId()];

		if (buf == NULL)
			buf = m_buffers[param->GetConfigId()] = new buffer(&m_monitor, (m_szarp_data_dir / param->GetSzarpConfig()->GetPrefix() / L"sz4").file_string());

		return buf;
	}

public:
	base(const std::wstring& szarp_data_dir) : m_szarp_data_dir(szarp_data_dir) {}

	template<class V, class T> void get_weighted_sum(TParam* param, const T& start, const T& end, weighted_sum<V, T>& sum) {
		buffer_for_param(param)->get_weighted_sum(param, start, end, sum);
	}

	template<class T> void search_data_right(TParam* param, const T& start, const T& end, const search_condition& condition) {
		buffer_for_param(param)->search_data_right(param, start, end, condition);
	}

	template<class T> void search_data_left(TParam* param, const T& start, const T& end, const search_condition& condition) {
		buffer_for_param(param)->search_data_left(param, start, end, condition);
	}

	void remove_param(TParam* param) {
		if (param->GetConfigId() >= m_buffers.size())
			return;

		buffer* buf = m_buffers[param->GetConfigId()];
		if (buf == NULL)
			return;

		buf->remove_param(param);
	}

	~base() {
		for (std::vector<buffer*>::iterator i = m_buffers.begin(); i != m_buffers.end(); i++)
			delete *i;
	}
};

}

#endif
