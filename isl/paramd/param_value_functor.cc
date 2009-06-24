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
/*
 * S³awomir Chy³ek 2008
 *
 * param_value_functor.cc - param value functor
 *
 * $Id$
 */

#include "param_value_functor.h"
#include "conversion.h"
#include "boost/lexical_cast.hpp"

boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> PTTParamValueGetter::operator()(){
	boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> ret;
	ret.get<0>() = m_proxy->getValue(m_ipc_ind, m_prec, VT_PROBE);
	ret.get<1>() = m_proxy->getValue(m_ipc_ind, m_prec, VT_MINUTE);
	ret.get<2>() = m_proxy->getValue(m_ipc_ind, m_prec, VT_MINUTES10);
	ret.get<3>() = m_proxy->getValue(m_ipc_ind, m_prec, VT_HOUR);
	return ret;
}

boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> PTTParamCombinedValueGetter::operator()(){
	boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> ret;
	ret.get<0>() = m_proxy->getCombinedValue(m_msw_ipc_ind, m_lsw_ipc_ind, m_prec, VT_PROBE);
	ret.get<1>() = m_proxy->getCombinedValue(m_msw_ipc_ind, m_lsw_ipc_ind, m_prec, VT_MINUTE);
	ret.get<2>() = m_proxy->getCombinedValue(m_msw_ipc_ind, m_lsw_ipc_ind, m_prec, VT_MINUTES10);
	ret.get<3>() = m_proxy->getCombinedValue(m_msw_ipc_ind, m_lsw_ipc_ind, m_prec, VT_HOUR);
	return ret;
}

boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> SzbaseParamValueGetter::operator()(){
	return boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring>(L"unknown", L"unknown", L"unknown", L"unknown");

	boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> ret = cache;

	time_t magic_offset = 60; //for meaner to write base

	if (time(NULL) - last_query_time > SZBASE_PROBE) {

		last_query_time = szb_round_time(time(NULL), PT_MIN10, 0) + magic_offset;

		bool ok = false;
		std::wstring error;

		time_t last_time = m_szbase->SearchLast(m_param->GetGlobalName(), ok);
		if (!ok) {
			ret = boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring>(L"unknown", L"unknown", L"unknown", L"unknown");
		} else {
			double vd = m_szbase->GetValue(m_param->GetGlobalName(), last_time, PT_MIN10, 0, NULL, ok, error);
			std::wstring v1 = L"unknown";
			if (ok)
				v1 = boost::lexical_cast<std::wstring>(vd);

			std::wstring v2 = L"unknown";
			vd = m_szbase->GetValue(m_param->GetGlobalName(), last_time, PT_HOUR, 0, NULL, ok, error);
			if (ok) 
				v2 = boost::lexical_cast<std::wstring>(vd);

			cache = ret = boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring>(v1, v1, v1, v2);
		}
	}

	return ret;
}
