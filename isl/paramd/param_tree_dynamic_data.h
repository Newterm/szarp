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
 * param_tree_dynamic_data.h - 
 *
 * $Id$
 */

#ifndef __PARAM_TREE_DYNAMIC_DATA_H__
#define __PARAM_TREE_DYNAMIC_DATA_H__

#include <boost/function.hpp>
#include <list>

#include "param_value_functor.h"

typedef std::map<xmlNodePtr, ParamValueGetterFunctor> ParamValueGetterMap;
typedef std::map<xmlNodePtr, ParamValueSetterFunctor> ParamValueSetterMap;

typedef boost::function<void ()> UpdateTrigger;

struct ParamDynamicTreeData {
	xmlDocPtr tree;
	ParamValueGetterMap get_map;
	ParamValueSetterMap set_map;
	std::list<UpdateTrigger> updateTriggers;
	TSzarpConfig *szarpConfig;
};

#endif

