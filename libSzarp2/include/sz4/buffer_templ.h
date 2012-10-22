#ifndef __SZ4_BUFFER_TEMPL_H__
#define __SZ4_BUFFER_TEMPL_H__
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

namespace sz4 {

template<template<typename DT, typename TT, class BT> class param_entry_type, class types, class data_type, class time_type> generic_param_entry* param_entry_build_t_3(base_templ<types>* base, TParam* param, const boost::filesystem::wpath &buffer_directory) {
	return new param_entry_in_buffer<param_entry_type, data_type, time_type, types>(base, param, buffer_directory);
}

template<template <typename DT, typename TT, class BT> class param_entry_type, class types, class data_type> generic_param_entry* param_entry_build_t_2(base_templ<types>* base, TParam* param, const boost::filesystem::wpath &buffer_directory) {
	switch (param->GetTimeType()) {
		case TParam::SECOND:
			return param_entry_build_t_3<param_entry_type, types, data_type, second_time_t>(base, param, buffer_directory);
		case TParam::NANOSECOND:
			return param_entry_build_t_3<param_entry_type, types, data_type, nanosecond_time_t>(base, param, buffer_directory);
	}
	return NULL;
}

template<template <typename DT, typename TT, class BT> class param_entry_type, class types> generic_param_entry* param_entry_build_t_1(base_templ<types>* base, TParam* param, const boost::filesystem::wpath &buffer_directory) {
	switch (param->GetDataType()) {
		case TParam::SHORT:
			return param_entry_build_t_2<param_entry_type, types, short>(base, param, buffer_directory);
		case TParam::INT:
			return param_entry_build_t_2<param_entry_type, types, int>(base, param, buffer_directory);
		case TParam::FLOAT:
			return param_entry_build_t_2<param_entry_type, types, float>(base, param, buffer_directory);
		case TParam::DOUBLE:
			return param_entry_build_t_2<param_entry_type, types, double>(base, param, buffer_directory);
	}
	return NULL;
}

template<class types> generic_param_entry* param_entry_build(base_templ<types> *base, TParam* param, const boost::filesystem::wpath &buffer_directory) {
	switch (param->GetSz4Type()) {
		case TParam::SZ4_REAL:
			return param_entry_build_t_1<real_param_entry_in_buffer, types>(base, param, buffer_directory);
		case TParam::SZ4_LUA_OPTIMIZED: {
			generic_param_entry* entry = param_entry_build_t_1<lua_optimized_param_entry_in_buffer, types>(base, param, buffer_directory);
			LuaExec::Param* exec_param = param->GetLuaExecParam();
			for (std::vector<LuaExec::ParamRef>::iterator i = exec_param->m_par_refs.begin();
					i != exec_param->m_par_refs.end();
					i++) {
				generic_param_entry* reffered_entry = base->get_param_entry(i->m_param);
				reffered_entry->add_reffering_param(entry);
			}
			return entry;
		}
		default:
		case TParam::SZ4_NONE:
			assert(false);
	}
}


template<class types> void buffer_templ<types>::remove_param(TParam* param) {
	if (m_param_ents.size() <= param->GetParamId())
		return;

	generic_param_entry* entry = m_param_ents[param->GetParamId()];
	if (!entry)
		return;

	entry->deregister_from_monitor(m_param_monitor);
	delete entry;

	m_param_ents[param->GetParamId()] = NULL;
}

template<class types> generic_param_entry* buffer_templ<types>::create_param_entry(TParam* param) {
	prepare_param(param);

	return param_entry_build(m_base, param, m_buffer_directory);
}

template<class types> void buffer_templ<types>::prepare_param(TParam* param) {
	if (param->GetSz4Type() != TParam::SZ4_NONE)
		return;

	if (param->GetType() == TParam::P_REAL) {
		param->SetSz4Type(TParam::SZ4_REAL);
		return;
	}

	if (param->IsDefinable())
		param->PrepareDefinable();

	if (param->GetType() == TParam::P_COMBINED) {
		param->SetSz4Type(TParam::SZ4_COMBINED);
		return;
	}

	if (param->GetType() == TParam::P_DEFINABLE) {
		param->SetSz4Type(TParam::SZ4_DEFINABLE);
		return;
	}

	if (param->GetType() == TParam::P_LUA) {
		LuaExec::Param* exec_param = new LuaExec::Param();
		param->SetLuaExecParam(exec_param);

		if (LuaExec::optimize_lua_param<ipk_container_type>(param, m_ipk_container)) 
			param->SetSz4Type(TParam::SZ4_LUA_OPTIMIZED);
		else
			param->SetSz4Type(TParam::SZ4_LUA);
		return;
	}
}

}

#endif
