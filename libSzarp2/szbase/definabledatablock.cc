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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "liblog.h"

// #include "szarp_config.h"

#include "szbbuf.h"
#include "szbname.h"
#include "szbfile.h"
#include "szbdate.h"

#include "definabledatablock.h"
#include "definablecalculate.h"

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"

namespace fs = boost::filesystem;

extern int szb_definable_error;	// kod ostatniego bledu

#ifdef DEBUG_PV
#include "stdarg.h"
void
debug_str(char * msg_str, ...)
{
    va_list args;
    fprintf(stderr, "DEBUG: ");
    fprintf(stderr, msg_str, args);
}
#endif

DefinableDatablock::DefinableDatablock(szb_buffer_t * b, TParam * p, int y, int m): CacheableDatablock(b, p, y, m)
{
	#ifdef KDEBUG
	sz_log(DATABLOCK_CREATION_LOG_LEVEL, "D: DefinableDatablock::DefinableDatablock(%ls, %d.%d)", param->GetName().c_str(), year, month);
	#endif

	time_t updatetime = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	this->AllocateDataMemory();

	int av_year, av_month;
	time_t end_date = szb_search_last(buffer, param);

	szb_time2my(end_date, &av_year, &av_month);

	if (LoadFromCache())
	{
		Refresh();
		return;
	}

	this->last_update_time = updatetime;
	
	// local copy for readability
	const std::wstring& formula = this->param->GetDrawFormula();
	int num_of_params = this->param->GetNumParsInFormula();

	#ifdef KDEBUG
	sz_log(9, "  f: %ls, n: %d", formula.c_str(), num_of_params);
	#endif

	const double* dblocks[num_of_params];
	TParam* params[num_of_params];

	// prevent removing blocks from cache
	szb_lock_buffer(this->buffer);

	int probes_to_compute;
	if  (num_of_params > 0) {
		probes_to_compute = this->GetBlocksUsedInFormula(dblocks, params, this->fixed_probes_count);
	} else {
		if (last_update_time > GetEndTime())
			probes_to_compute = fixed_probes_count = max_probes;
		else if (GetStartTime() > last_update_time)
			probes_to_compute = fixed_probes_count = 0;
		else
			probes_to_compute = szb_probeind(last_update_time) + 1;
	}

	assert(this->fixed_probes_count <= this->max_probes);

	double pw = pow(10, param->GetPrec());

	SZBASE_TYPE  stack[DEFINABLE_STACK_SIZE]; // stack for calculatinon of formula

#ifdef KDEBUG
	sz_log(10, "V szb_definable_calculate_block: probes_to_compute: %d, max: %d, N? %s",
		probes_to_compute, this->max_probes, param->IsNUsed() ? "YES" : "NO");
#endif	

	int i = 0;
	time_t time = probe2time(0, year, month);
	for (; i < probes_to_compute; i++, time += SZBASE_DATA_SPAN) {
		this->data[i] = szb_definable_calculate(b, stack, dblocks, params, formula, i, num_of_params, time, param) / pw;

		if (!IS_SZB_NODATA(this->data[i])) {
			if(this->first_data_probe_index < 0)
				this->first_data_probe_index = i;
			this->last_data_probe_index = i;
		}

		if (0 != szb_definable_error) { // error
			sz_log(1,
				"D: szb_definable_calculate_block: error %d for param: %ls date: %d.%d",
				szb_definable_error, this->param->GetName().c_str(), this->year, this->month);
			szb_unlock_buffer(this->buffer);
			NOT_INITIALIZED;
		}
	}

	if (this->first_data_probe_index >= 0) {
		assert(!IS_SZB_NODATA(this->data[this->GetFirstDataProbeIdx()]));
		assert(!IS_SZB_NODATA(this->data[this->GetLastDataProbeIdx()]));
	}

	for (; i < this->max_probes; i++)
		this->data[i] = SZB_NODATA;

	szb_unlock_buffer(this->buffer);

	return;
}

int
DefinableDatablock::GetBlocksUsedInFormula(const double** blocks, TParam** params, int &fixedprobes)
{
	int probes = 0;
	int num_of_params = this->param->GetNumParsInFormula();
	TParam ** f_cache = this->param->GetFormulaCache();

	fixedprobes = max_probes;

	for (int i = 0; i < num_of_params; i++) {

		szb_datablock_t* block = szb_get_datablock(this->buffer, f_cache[i], this->year, this->month);

		if (block == NULL) {
			blocks[i] = NULL;
			fixedprobes = 0;
			continue;
		}
		block->Refresh();

		blocks[i] = block->GetData(false);
		params[i] = block->param;

		switch (f_cache[i]->GetType()) {
			case TParam::P_REAL:
			case TParam::P_COMBINED:
			case TParam::P_DEFINABLE:
			case TParam::P_LUA:
				fixedprobes = std::min(fixedprobes, block->GetFixedProbesCount());
				block_timestamp = std::min(block_timestamp, block->GetBlockTimestamp());
				break;
		}
	}

	if (GetEndTime() < last_update_time)
		probes = max_probes;
	else if (GetStartTime() > last_update_time)
		probes = 0;
	else
		probes = szb_probeind(last_update_time) + 1;

	return probes;
}

void
DefinableDatablock::Refresh()
{
	// block is full - no more probes can be load
	if (this->fixed_probes_count == this->max_probes)
		return;

	time_t updatetime = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	if (this->last_update_time == updatetime)
		return;

	this->last_update_time = updatetime;

	sz_log(DATABLOCK_REFRESH_LOG_LEVEL, "DefinableDatablock::Refresh() '%ls'", this->GetBlockRelativePath().c_str());

	// local copy for readability
	const std::wstring& formula = this->param->GetDrawFormula();
	int num_of_params = this->param->GetNumParsInFormula();

	const double* dblocks[num_of_params];
	TParam* params[num_of_params];

	// prevent removing blocks from cache
	szb_lock_buffer(this->buffer);

	int new_probes_c = 0;
	int new_fixed_probes;

	if (num_of_params > 0) {
		new_probes_c = GetBlocksUsedInFormula(dblocks, params, new_fixed_probes);
	} else {
		if (last_update_time > GetEndTime())
			new_probes_c = new_fixed_probes = max_probes;
		else
			new_probes_c = new_fixed_probes = szb_probeind(szb_search_last(buffer, param)) + 1;
	}
	
	assert(new_fixed_probes >= this->fixed_probes_count);

	SZBASE_TYPE stack[DEFINABLE_STACK_SIZE]; // stack for calculatinon of formula

	double pw = pow(10, this->param->GetPrec());

	if (this->first_data_probe_index >= int(fixed_probes_count))
		first_data_probe_index = -1;
	if (this->last_data_probe_index >= int(fixed_probes_count))
		last_data_probe_index = -1;

	time_t time = probe2time(0, year, month);
	for (int i = this->fixed_probes_count; i < new_probes_c; i++, time += SZBASE_DATA_SPAN) {
		this->data[i] = szb_definable_calculate(buffer, stack, dblocks, params, formula, i, num_of_params, time, param) / pw;

		if (!IS_SZB_NODATA(this->data[i])) {
			if(this->first_data_probe_index < 0 || i < this->first_data_probe_index)
				this->first_data_probe_index = i;
			this->last_data_probe_index = i;
		}

		if (0 != szb_definable_error) { // error
			sz_log(1,
				"D: szb_definable_calculate_block: error %d for param: %ls date: %d.%d",
				szb_definable_error, this->param->GetName().c_str(), this->year, this->month);
			break;
		}
	}

	if (this->first_data_probe_index >= 0) {
		assert(!IS_SZB_NODATA(this->data[this->GetFirstDataProbeIdx()]));
		assert(!IS_SZB_NODATA(this->data[this->GetLastDataProbeIdx()]));
	}

	this->fixed_probes_count = new_fixed_probes;

	szb_unlock_buffer(buffer);

	return;
}

bool DefinableDatablock::IsCachable()
{
	return fixed_probes_count > 0;
}
