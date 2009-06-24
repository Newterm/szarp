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
 * param_value_functor.h - param value functor
 *
 * $Id$
 */

#ifndef __PARAM_VALUE_FUNCTOR
#define __PARAM_VALUE_FUNCTOR

#include <libxml/parser.h>
#include <boost/function.hpp>
#include <boost/tuple/tuple.hpp>

#include "szbase/szbbase.h"
#include "ptt2xml.h"
#include "conversion.h"
#include "probes_types.h"

typedef boost::function<boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> ()> ParamValueGetterFunctor;
typedef boost::function<int (std::wstring)> ParamValueSetterFunctor;

template<class T>
void callMyArgument(T x){
	x();
};

class PTTProxyUpdater {
	protected:
		PTTParamProxy * m_proxy;
	public:
		PTTProxyUpdater(PTTParamProxy * proxy) : m_proxy(proxy) {};
		void operator()() {
			m_proxy->update();
		};
};

class DummyHandler {
	public:
		void operator()(){};
		int operator()(std::wstring){return -1;};
};

class SzbaseParamValueGetter {
	protected:
		Szbase * m_szbase;
		TParam * m_param;
		time_t last_query_time;
		boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> cache;
	public:
		SzbaseParamValueGetter(Szbase * szbase, TParam * param) : m_szbase(szbase), m_param(param), last_query_time(0) {};
		boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> operator()();
};

class PTTParamValueGetter {
	protected:
		PTTParamProxy * m_proxy;
		int m_ipc_ind;
		int m_prec;
	public:
		PTTParamValueGetter(PTTParamProxy * proxy, int ipc_ind, int prec) : m_proxy(proxy), m_ipc_ind(ipc_ind), m_prec(prec) {};
		boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> operator()();
};

class PTTParamValueSetter {
	protected:
		PTTParamProxy * m_proxy;
		int m_ipc_ind;
		int m_prec;
	public:
		PTTParamValueSetter(PTTParamProxy * proxy, int ipc_ind, int prec) : m_proxy(proxy), m_ipc_ind(ipc_ind), m_prec(prec) {};
		int operator()(std::wstring value){
			return m_proxy->setValue(m_ipc_ind, m_prec, value);
		};
};

class PTTParamCombinedValueGetter {
	protected:
		PTTParamProxy * m_proxy;
		int m_lsw_ipc_ind;
		int m_msw_ipc_ind;
		int m_prec;
	public:
		PTTParamCombinedValueGetter(PTTParamProxy * proxy, int lsw_ipc_ind, int msw_ipc_ind, int prec) : m_proxy(proxy), m_lsw_ipc_ind(lsw_ipc_ind), m_msw_ipc_ind(msw_ipc_ind), m_prec(prec) {};
		boost::tuple<std::wstring, std::wstring, std::wstring, std::wstring> operator()();
};

#endif

