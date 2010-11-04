#ifndef __LOPTDATABLOCK_H__
#define __LOPTDATABLOCK_H__

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

#include <boost/shared_ptr.hpp>

#include "luadatablock.h"

LuaDatablock *create_lua_data_block(szb_buffer_t *b, TParam* p, int y, int m);

#ifdef LUA_PARAM_OPTIMISE

namespace LuaExec {

class ExecutionEngine;

};

class LuaOptDatablock : public LuaDatablock 
{
	void CalculateValues(LuaExec::ExecutionEngine *ee, int end_probe);
public:
	LuaExec::Param *exec_param;
	LuaOptDatablock(szb_buffer_t * b, TParam * p, int y, int m);
	virtual void FinishInitialization();
	virtual void Refresh();
};

#endif

#endif
