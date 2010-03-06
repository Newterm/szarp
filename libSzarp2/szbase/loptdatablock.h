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
 *
 * loptdatablock
 * $Id: cacheabledatablock.h 267 2010-01-11 09:06:34Z reksi0 $
 */

#ifndef __LOPTDATABLOCK_H__
#define __LOPTDATABLOCK_H__

#include "config.h"

#ifdef LUA_PARAM_OPTIMISE

#include "cacheabledatablock.h"

namespace LuaExec {
	class Param;
	class ExecutionEngine;
};

class LuaOptDatablock : public CacheableDatablock
{
	void CalculateValues(LuaExec::ExecutionEngine *ee);
public:
	LuaExec::Param *exec_param;
	LuaOptDatablock(szb_buffer_t * b, TParam * p, int y, int m);
	virtual void Refresh();
};

#endif

#endif
