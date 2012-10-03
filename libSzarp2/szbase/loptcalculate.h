#ifndef LOPTCALCULATE_H__
#define LOPTCALCULATE_H__
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

#include "szarp_base_common/lua_param_optimizer.h"

namespace LuaExec {

struct SzbaseParam : public Param {
        std::map<szb_buffer_t*, time_t> m_last_update_times;
};

class SzbaseExecutionEngine : public ExecutionEngine {
	szb_buffer_t* m_buffer;
	std::vector<szb_buffer_t*> m_buffers;
	struct ListEntry {
		time_t start_time;
		time_t end_time;
		szb_block_t *block;
	};
	std::vector<std::vector<std::list<ListEntry> > > m_blocks;
	std::vector<std::vector<std::list<ListEntry>::iterator> > m_blocks_iterators;
	std::vector<double> m_vals;
	Param* m_param;
	bool m_fixed;
	ListEntry GetBlockEntry(size_t param_index, time_t t, SZB_BLOCK_TYPE bt);
	szb_block_t* AddBlock(size_t param_index, time_t t, std::list<ListEntry>::iterator& i, SZB_BLOCK_TYPE bt);
	szb_block_t* SearchBlockLeft(size_t param_index, time_t t, std::list<ListEntry>::iterator& i, SZB_BLOCK_TYPE bt);
	szb_block_t* SearchBlockRight(size_t param_index, time_t t, std::list<ListEntry>::iterator& i, SZB_BLOCK_TYPE bt);
	szb_block_t* GetBlock(size_t param_index, time_t time, SZB_BLOCK_TYPE bt);
public:
	SzbaseExecutionEngine(szb_buffer_t *buffer, Param *param);

	void CalculateValue(time_t t, SZARP_PROBE_TYPE probe_type, double &val, bool &fixed);

	virtual double Value(size_t param_index, const double& time, const double& period_type);
	double ValueBlock(ParamRef& ref, const time_t& time, SZB_BLOCK_TYPE block_type);
	double ValueAvg(ParamRef& ref, const time_t& time, const double& period_type);

	std::vector<double>& Vars();

	~SzbaseExecutionEngine();
};

Param* optimize_lua_param(TParam* p);

}

#endif
