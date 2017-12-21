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
#ifndef __SZ4_BUFFER_TEMPL_H__
#define __SZ4_BUFFER_TEMPL_H__

#include <algorithm>

#include "conversion.h"
#include "szarp_base_common/lua_strings_extract.h"
#include "factory.h"

namespace sz4 {

template<class base> buffer_templ<base>::buffer_templ(base* _base, SzbParamMonitor* param_monitor, ipk_container_type* ipk_container, const std::wstring& prefix, const std::wstring& buffer_directory)
			: m_base(_base), m_param_monitor(param_monitor), m_ipk_container(ipk_container), m_buffer_directory(buffer_directory) {

	TParam* heart_beat_param = m_ipk_container->GetParam(prefix + L":Status:Meaner3:program_uruchomiony");
	if (heart_beat_param)
		m_heart_beat_entry = get_param_entry(heart_beat_param);
	else
		m_heart_beat_entry = NULL;
}

template<class base> generic_param_entry* buffer_templ<base>::get_param_entry(TParam* param) {
	if (m_param_ents.size() <= param->GetParamId())
		m_param_ents.resize(param->GetParamId() + 1, NULL);

	generic_param_entry* entry = m_param_ents[param->GetParamId()];
	if (entry == NULL) {
		entry = m_param_ents[param->GetParamId()] = create_param_entry(param);

		auto live_cache = m_base->get_live_cache();
		if (live_cache)
			live_cache->register_cache_observer(param, entry);

		entry->register_at_monitor(m_param_monitor);
	}
	return entry;
}


template<template<typename DT, typename TT, class BT> class param_entry_type, class base> generic_param_entry* param_entry_build(base* _base, TParam* param, const boost::filesystem::wpath &buffer_directory) {
	typedef typename base::param_factory factory;
	return factory().template create<param_entry_type, base>(_base, param, buffer_directory);
}

template<class base> generic_param_entry* param_entry_build(base *_base, TParam* param, const boost::filesystem::wpath &buffer_directory) {
	switch (param->GetSz4Type()) {
		case Sz4ParamType::REAL:
			return param_entry_build<real_param_entry_in_buffer, base>(_base, param, buffer_directory);
		case Sz4ParamType::COMBINED: {
			param->SetDataType(TParam::INT);

			auto entry = param_entry_build<combined_param_entry_in_buffer, base>(_base, param, buffer_directory);

			TParam **f_cache = param->GetFormulaCache();
			int num_of_params = param->GetNumParsInFormula();
			for (int i = 0; i < num_of_params; i++) {
				generic_param_entry* refferred_entry = _base->get_param_entry(f_cache[i]);
				refferred_entry->add_refferring_param(entry);
				entry->add_refferred_param(refferred_entry);
			}

			return entry;
		}
		case Sz4ParamType::LUA_OPTIMIZED: {
			param->SetDataType(TParam::DOUBLE);

			auto entry = param_entry_build<lua_optimized_param_entry_in_buffer, base>(_base, param, buffer_directory);

			LuaExec::Param* exec_param = param->GetLuaExecParam();
			for (std::vector<LuaExec::ParamRef>::iterator i = exec_param->m_par_refs.begin();
					i != exec_param->m_par_refs.end();
					i++) 
				if (i->m_param != param) {
					generic_param_entry* refferred_entry = _base->get_param_entry(i->m_param);
					refferred_entry->add_refferring_param(entry);
					entry->add_refferred_param(refferred_entry);
				}
			return entry;
		}
		case Sz4ParamType::LUA: {
			param->SetDataType(TParam::DOUBLE);

			auto entry = param_entry_build<lua_param_entry_in_buffer, base>(_base, param, buffer_directory);

			std::wstring formula = SC::U2S(param->GetLuaScript());
			std::vector<std::wstring> strings;
			extract_strings_from_formula(formula.c_str(), strings);

			typename base::ipk_container_type* ipk_container = _base->get_ipk_container();
			for (std::vector<std::wstring>::iterator i = strings.begin(); i != strings.end(); i++) {
				if (std::count(i->begin(), i->end(), L':') != 3)
					continue;

				TParam * rparam = ipk_container->GetParam(*i);
				if (rparam == NULL || param == rparam)
					continue;

				generic_param_entry* refferred_entry = _base->get_param_entry(rparam);
				refferred_entry->add_refferring_param(entry);
				entry->add_refferred_param(refferred_entry);
			}
			return entry;
		}
		case Sz4ParamType::DEFINABLE: {
			param->SetDataType(TParam::DOUBLE);

			auto entry = param_entry_build<rpn_param_entry_in_buffer, base>(_base, param, buffer_directory);

			TParam **f_cache = param->GetFormulaCache();
			int num_of_params = param->GetNumParsInFormula();
			for (int i = 0; i < num_of_params; i++) {
				generic_param_entry* refferred_entry = _base->get_param_entry(f_cache[i]);
				refferred_entry->add_refferring_param(entry);
				entry->add_refferred_param(refferred_entry);
			}

			return entry;
		}
		default:
		case Sz4ParamType::NONE:
			assert(false);
	}
}

template<class base> void buffer_templ<base>::get_heartbeat_last_time(second_time_t& t) {
	auto live = m_base->get_live_cache();
	if (live && live->get_last_meaner_time(m_heart_beat_entry->get_param()->GetConfigId(), t))
		return;
	m_heart_beat_entry->get_last_time(t);
}

template<class base> void buffer_templ<base>::get_heartbeat_last_time(nanosecond_time_t& t) {
	m_heart_beat_entry->get_last_time(t);

	auto live = m_base->get_live_cache();
	if (live && live->get_last_meaner_time(m_heart_beat_entry->get_param()->GetConfigId(), t))
		return;
	m_heart_beat_entry->get_last_time(t);
}

template<class base> void buffer_templ<base>::remove_param(TParam* param) {
	if (m_param_ents.size() <= param->GetParamId())
		return;

	generic_param_entry* entry = m_param_ents[param->GetParamId()];
	if (!entry)
		return;

	while (entry->referring_params().size()) {
		auto p = (*entry->referring_params().begin())->get_param();
		remove_param(p);
	}

	entry->deregister_from_monitor(m_param_monitor);
	delete entry;

	param->SetSz4Type(Sz4ParamType::NONE);

	m_param_ents[param->GetParamId()] = NULL;
}

template<class base> generic_param_entry* buffer_templ<base>::create_param_entry(TParam* param) {
	prepare_param(param);

	return param_entry_build(m_base, param, m_buffer_directory);
}

template<class base> void buffer_templ<base>::prepare_param(TParam* param) {
	if (param->GetSz4Type() != Sz4ParamType::NONE)
		return;

	if (param->GetType() == ParamType::REAL) {
		param->SetSz4Type(Sz4ParamType::REAL);
		return;
	}

	if (param->IsDefinable())
		param->PrepareDefinable();

	if (param->GetType() == ParamType::COMBINED) {
		param->SetSz4Type(Sz4ParamType::COMBINED);
		param->SetDataType(TParam::INT);
		return;
	}

	if (param->GetType() == ParamType::DEFINABLE) {
		param->SetSz4Type(Sz4ParamType::DEFINABLE);
		return;
	}

	if (param->GetType() == ParamType::LUA) {
		LuaExec::Param* exec_param = new LuaExec::Param();
		param->SetLuaExecParam(exec_param);

		if (LuaExec::optimize_lua_param<ipk_container_type>(param, m_ipk_container)) 
			param->SetSz4Type(Sz4ParamType::LUA_OPTIMIZED);
		else
			param->SetSz4Type(Sz4ParamType::LUA);
		return;
	}
}

template<class base> buffer_templ<base>::~buffer_templ() {
	for (std::vector<generic_param_entry*>::iterator i = m_param_ents.begin(); i != m_param_ents.end(); i++) {
		if (*i)
			(*i)->deregister_from_monitor(m_param_monitor);
		delete *i;
	}
}

}

#endif
