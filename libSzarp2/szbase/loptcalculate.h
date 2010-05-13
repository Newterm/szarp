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

namespace LuaExec {

class ExecutionEngine {
	szb_buffer_t* m_buffer;
	struct BlockListEntry {
		time_t start;
		time_t end;
		szb_datablock_t *block;
		BlockListEntry(time_t start_, time_t end_, szb_datablock_t* block_) : start(start_), end(end_), block(block_) {}
	};
	std::vector<TParam*> m_params;
	std::vector<std::list<BlockListEntry> > m_blocks;
	std::vector<std::list<BlockListEntry>::iterator> m_blocks_iterators;
	std::vector<double> m_vals;
	Param* m_param;
	bool m_fixed;
	BlockListEntry CreateBlock(size_t param_index, time_t t);
	BlockListEntry& AddBlock(size_t param_index, time_t t, std::list<BlockListEntry>::iterator& i);
	BlockListEntry& SearchBlockLeft(size_t param_index, time_t t, std::list<BlockListEntry>::iterator& i);
	BlockListEntry& SearchBlockRight(size_t param_index, time_t t, std::list<BlockListEntry>::iterator& i);
	BlockListEntry& GetBlock(size_t param_index, time_t time);
public:
	ExecutionEngine(szb_buffer_t *buffer, 

	void CalculateValue(time_t t, SZARP_PROBE_TYPE probe_type, double &val, bool &fixed);

	double Value(size_t param_index, const double& time, const double& period_type);

	std::vector<double>& Vars();

	~ExecutionEngine();
};

}

#endif
