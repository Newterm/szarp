#ifndef __SZ4_PARAM_ENTRY_FACTORY_H__
#define __SZ4_PARAM_ENTRY_FACTORY_H__
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

#include "sz4/param_entry.h"
#include "sz4/factory.h"

namespace sz4 {

struct param_entry_factory {
	template<template<typename DT, typename TT, class BT> class entry_type, class base> struct builder {
		template<class _data, class _time, class ...Args> static generic_param_entry* op(Args... args) {
			return new param_entry_in_buffer<entry_type, _data, _time, base>(args...);
		};
	};

	template<
		template<typename DT, typename TT, class BT> class entry_type,
		typename base 
	>
	generic_param_entry* create(base* _base, TParam* param, const boost::filesystem::wpath &buffer_directory) const {
		return factory<
			generic_param_entry,
			builder<entry_type, base>
				>::op(param, _base, param, buffer_directory);
	}
};

}

#endif
