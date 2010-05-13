/* 
  SZARP: SCADA software 

*/
/*
 * Liczenie parametrow definiowalnych - nowa wersja
 * $Id$
 *
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

/** Search first available probe in given period.
 * @param buffer pointer to cache buffer
 * @param param  parameter
 * @param start_time begin of period
 * @param end_time   end of period
 * @param direction  direction of search
 * @return date of first available probe
 */
time_t
szb_definable_search_data(szb_buffer_t * buffer, TParam * param, 
	time_t start_time, time_t end_time, int direction)
{
#ifdef KDEBUG
	sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: %s, s:%ld, e:%ld, d:%d",
		param->GetName(),
		(unsigned long int) start_time, (unsigned long int) end_time,
		direction);
#endif

	time_t first_available = buffer->first_av_date; 
	time_t last_available = szb_search_last(buffer, param);

#ifdef KDEBUG
	sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: fad:%ld, lad:%ld",
		(unsigned long int) first_available,
		(unsigned long int) last_available);
#endif

	if (param->IsConst()) {
		if (start_time >= first_available && start_time <= last_available) {
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", start_time);
#endif
			return start_time;
		}

		switch (direction) {
			case -1:
				if (end_time < 0 || end_time < first_available)
					end_time = first_available;
				if (end_time < 0 || end_time > last_available) {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
					return -1;
				}

				if (start_time < 0 || start_time > last_available)
					start_time = last_available;
				if (start_time >= 0 && start_time > end_time) {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", start_time);
#endif
					return start_time;
				} else {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
					return -1;
				}
				break;
			case 1:
				if (end_time < 0 || end_time > last_available)
					end_time = last_available;
				if (end_time < first_available) {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
					return -1;
				}

				if (start_time < 0 || start_time < first_available)
					start_time = first_available;
				if (start_time < 0 || start_time > last_available) {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
					return -1;
				} else {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", start_time);
#endif
					return start_time;
				}
			default:
#ifdef KDEBUG
				sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
				return -1;
		}
	}

#ifdef DEBUG
	debug_str("ndef.c (641): last_available = %s\n", ctime(&last_available));
#endif

	if ((start_time >= first_available && start_time <= last_available)
			&& !IS_SZB_NODATA(szb_get_data(buffer, param, start_time))) {
#ifdef KDEBUG
		sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", start_time);
#endif
		return start_time;
	}

	time_t time = -1;

	switch (direction) {
		case -1: // search left
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_definable_search_last: searching left");
#endif
			if (end_time < 0 || end_time < first_available)
				end_time = first_available;
			if (end_time < 0 || end_time > last_available) {
#ifdef KDEBUG
				sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
				return -1;
			}

			if (start_time < 0 || start_time > last_available)
				start_time = last_available;
			if (start_time >= 0 && start_time > end_time) {
				int probe_n, month = 0, year = 0;
				int new_year, new_month;
				szb_datablock_t * block = NULL;

				szb_time2my(start_time, &year, &month);
#ifdef KDEBUG
				sz_log(9, "szb_definable_search_data: s_t:%ld, e_t:%ld",
					(unsigned long int) start_time, (unsigned long int) end_time);
#endif
				const SZBASE_TYPE * data = NULL;
				for (time = start_time; time >= end_time; time -= SZBASE_PROBE) {
					szb_time2my(time, &new_year, &new_month);
					if (block == NULL || new_month != month || new_year != year) {
						block = szb_get_block(buffer, param, new_year, new_month);
						if (block != NULL)
							data = block->GetData();
						if (NULL == block || block->GetFirstDataProbeIdx() < 0) {
							time = probe2time(0, new_year, new_month);
							continue;
						}
						year = new_year;
						month = new_month;
					}
					/* check for data at index */
					probe_n = szb_probeind(time);
		
					assert(!IS_SZB_NODATA(data[block->GetFirstDataProbeIdx()]));
					assert(!IS_SZB_NODATA(data[block->GetLastDataProbeIdx()]));
		
					if(probe_n > block->GetLastDataProbeIdx()) { //If index is after last data jump to last data in next iteration
						time = probe2time(block->GetLastDataProbeIdx() + 1, new_year, new_month);
						continue;
					}
					if(probe_n < block->GetFirstDataProbeIdx()) { //If index is before first data jump to the previous block in next iteration
						time = probe2time(0, new_year, new_month);
						continue;
					}
		
					if ( !IS_SZB_NODATA(data[probe_n]) ) {
#ifdef KDEBUG
						sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", time);
#endif
						return time;
					}
				}
			}
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
			return -1;		
		case 1: // search right
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_definable_search_data: searching right");
#endif
			if (end_time < 0)
				end_time = last_available;

			if (end_time < first_available) {
#ifdef KDEBUG
				sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
				return -1;
			}

			if (start_time < 0 || start_time < first_available)
				start_time = first_available;
			if (start_time < 0 || start_time > last_available) {
#ifdef KDEBUG
				sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
				return -1;
			}

			if (end_time >= 0) {
				int probe_n, month = 0, year = 0;
				int new_year, new_month;
				szb_datablock_t * block = NULL;

				szb_time2my(start_time, &year, &month);

				const SZBASE_TYPE * data = NULL;
				for (time = start_time; time <= end_time; time += SZBASE_PROBE) {
					szb_time2my(time, &new_year, &new_month);
					if (block == NULL || new_month != month || new_year != year) {
						block = szb_get_block(buffer, param, new_year, new_month);
						if (NULL != block)
							data = block->GetData();
						if (NULL == block || block->GetFirstDataProbeIdx() < 0) {
							time = probe2time(szb_probecnt(new_year, new_month) - 1, new_year, new_month);
							continue;
						}
						data = block->GetData();
						year = new_year;
						month = new_month;
					}
					probe_n = szb_probeind(time);
					assert(probe_n >= 0);
			
					assert(!IS_SZB_NODATA(data[block->GetFirstDataProbeIdx()]));
					assert(!IS_SZB_NODATA(data[block->GetLastDataProbeIdx()]));
			
					if(probe_n > block->GetLastDataProbeIdx()) { //If index is after last data jump to the next block
						time = probe2time(block->max_probes - 1, new_year, new_month);
						continue;
					}
					if(probe_n < block->GetFirstDataProbeIdx()) { //If index is before first data jump first data in next iteration
						time = probe2time(block->GetFirstDataProbeIdx() - 1, new_year, new_month);
						continue;
					}
			
					if (!IS_SZB_NODATA(block->GetData()[probe_n])) {
#ifdef KDEBUG
						sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", time);
#endif
						return time;
					}
				}
			}
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
			return -1;
		default:
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
			return -1;
	}

	return time;
}

DefinableDatablock::DefinableDatablock(szb_buffer_t * b, TParam * p, int y, int m): CacheableDatablock(b, p, y, m)
{
	#ifdef KDEBUG
	sz_log(DATABLOCK_CREATION_LOG_LEVEL, "D: DefinableDatablock::DefinableDatablock(%ls, %d.%d)", param->GetName().c_str(), year, month);
	#endif

	time_t updatetime = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	this->AllocateDataMemory();

	if (year < this->buffer->first_av_year)
		NOT_INITIALIZED;

	if (year == this->buffer->first_av_year && month < this->buffer->first_av_month)
		NOT_INITIALIZED;

	int av_year, av_month;
	time_t end_date = szb_search_last(buffer, param);

	szb_time2my(end_date, &av_year, &av_month);

	if (year > av_year || (year == av_year && month > av_month))
		NOT_INITIALIZED;

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
	if (num_of_params > 0) {
		probes_to_compute = this->GetBlocksUsedInFormula(dblocks, params, this->first_non_fixed_probe);
	} else {
		if (this->last_update_time > this->GetBlockLastDate())
			probes_to_compute = this->first_non_fixed_probe = this->max_probes;
		else if (this->last_update_time < this->GetBlockBeginDate()) {
			NOT_INITIALIZED;
		} else
			probes_to_compute = this->first_non_fixed_probe = szb_probeind(end_date) + 1;
	}

	assert(this->first_non_fixed_probe <= this->max_probes);

	if (probes_to_compute <= 0)
		NOT_INITIALIZED;

	/* if N is used or no params in formula we must calculate probes to last_av_date */
// 	if (param->IsNUsed() || 0 == num_of_params) {
// 		this->probes_c = GetProbesBeforeLastAvDate();
// 	}

	double pw = pow(10, param->GetPrec());

	SZBASE_TYPE  stack[DEFINABLE_STACK_SIZE]; // stack for calculatinon of formula

#ifdef KDEBUG
	sz_log(10, "V szb_definable_calculate_block: probes_to_compute: %d, max: %d, N? %s",
		probes_to_compute, this->max_probes, param->IsNUsed() ? "YES" : "NO");
#endif	

	int i = 0;
	time_t time = probe2time(0, year, month);
	for (; i < probes_to_compute; i++, time += SZBASE_PROBE) {
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

	for (; i < this->max_probes; i++) {
		this->data[i] = SZB_NODATA;
	}

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

		szb_datablock_t* block = szb_get_block(this->buffer, f_cache[i], this->year, this->month);

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
				probes = probes > block->GetLastDataProbeIdx() + 1 ? probes : block->GetLastDataProbeIdx() + 1;
				fixedprobes = fixedprobes < block->GetFixedProbesCount() ? fixedprobes : block->GetFixedProbesCount();
				this->block_timestamp = this->block_timestamp > block->GetBlockTimestamp() ? this->block_timestamp : block->GetBlockTimestamp();
				break;
		}
	}

	if(probes < fixedprobes)
		probes = fixedprobes;

	if(this->GetBlockLastDate() < this->last_update_time) {
		probes = max_probes;
	} else if (this->GetBlockBeginDate() < this->last_update_time) {
		int tmp = szb_probeind(szb_search_last(buffer, this->param)) + 1;
		if(tmp > probes)
			probes = tmp;
	}

	return probes;
}

void
DefinableDatablock::Refresh()
{
	// block is full - no more probes can be load
	if (this->first_non_fixed_probe == this->max_probes)
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
		if (this->last_update_time > this->GetBlockLastDate())
			new_probes_c = new_fixed_probes = this->max_probes;
		else
			new_probes_c = new_fixed_probes = szb_probeind(szb_search_last(buffer, param)) + 1;
	}
	
	assert(new_probes_c > 0);

	assert(new_fixed_probes >= this->first_non_fixed_probe);

	SZBASE_TYPE stack[DEFINABLE_STACK_SIZE]; // stack for calculatinon of formula

	double pw = pow(10, this->param->GetPrec());

	if (this->first_data_probe_index >= first_non_fixed_probe)
		first_data_probe_index = -1;
	if (this->last_data_probe_index >= first_non_fixed_probe)
		last_data_probe_index = -1;

	time_t time = probe2time(0, year, month);
	for (int i = this->first_non_fixed_probe; i < new_probes_c; i++, time += SZBASE_PROBE) {
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

	this->first_non_fixed_probe = new_fixed_probes;

	szb_unlock_buffer(buffer);

	return;
}

bool DefinableDatablock::IsCachable()
{
	return first_non_fixed_probe > 0;
}
