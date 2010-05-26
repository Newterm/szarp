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
 * realdatablock
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

#include "conversion.h"

#include "szbbuf.h"
#include "szbname.h"
#include "szbfile.h"
#include "szbdate.h"
#include "szbhash.h"
#include "szbbase.h"
//#include "szb_python.h"
#include "include/szarp_config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "liblog.h"
#include "szbbuf.h"

#include "realdatablock.h"

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"

namespace fs = boost::filesystem;

// DEBUG
// #define DD
// #define DDD
// 

/** Load block from disk file and return pointer to it. 
 * @param buffer buffer pointer
 * @param param pointer to parameter
 * @param year year (4 digits)
 * @param month month (from 1 to 12)
 * @return pointer to block loaded, NULL if not found or on error, check
 * buffer->last_err for error code
 */
RealDatablock::RealDatablock(szb_buffer_t * b, TParam * p, int y, int m) :
	szb_datablock_t(b, p, y, m), probes_from_file(0) {
#ifdef KDEBUG
	sz_log(DATABLOCK_CREATION_LOG_LEVEL, "D: RealDatablock::RealDatablock: (%s, %d.%d)", param->GetName(), year, month);
#endif

	//Init the empty field for fully empty blocks
	if (!empty_field_initialized) {
		int x= MAX_PROBES;
		empty_field = new SZBASE_TYPE[x];
		for (int i = 0; i < x; i++)
			empty_field[i] = SZB_NODATA;
		empty_field_initialized = true;
	}

	//Get last database update time - `now`
	this->last_update_time = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	if (szb_file_exists(this->GetBlockFullPath().c_str(), &this->block_timestamp)) {
		if (!LoadFromFile()) //If file exists load data from in
			assert(false);
		//CBB: something more reasonable
		if (this->fixed_probes_count == this->max_probes)
			return; //Block is full
	} else {
		if (this->GetEndTime() < buffer->first_av_date)
			NOT_INITIALIZED; //Block is before first_av_date - not creating empty block
	}

	int ly, lm;
	szb_time2my(szb_search_last(buffer, param), &ly, &lm);

	if ( (year < ly || (year == ly && month < lm)) || this->last_update_time > this->GetEndTime()) {
		this->fixed_probes_count = this->max_probes; //If there are future data or the end of block is before `now`
	} else {
		int tmp = szb_probeind(this->last_update_time) + 1;
		this->fixed_probes_count = this->fixed_probes_count > tmp ? this->fixed_probes_count : tmp; //we have all read data or untill `now`
	}

	return;
}

RealDatablock::~RealDatablock() {
}

bool RealDatablock::LoadFromFile() {
	int to_read = max_probes * sizeof(SZB_FILE_TYPE);
	SZB_FILE_TYPE * buf = (SZB_FILE_TYPE *) malloc(to_read);
	if (NULL == buf) {
		buffer->last_err = SZBE_OUTMEM;
		return false;
	}

#ifdef MINGW32
	/* Stupid, isnt't it? */
	int fd = open(SC::S2A(GetBlockFullPath()).c_str(), O_RDONLY | O_BINARY);
#else
	int fd = open(SC::S2A(GetBlockFullPath()).c_str(), O_RDONLY);
#endif

	if (fd == -1) {
		free(buf);
		buffer->last_err = SZBE_SYSERR;
		return false;
	}

	int i;
	while (to_read > 0) {
		i = read(fd, (char *) buf + (max_probes * sizeof(SZB_FILE_TYPE)) - to_read, to_read);
		if ((i >= 0) && (i < to_read)) {
			lseek(fd, i, SEEK_SET);
		}
		if (i == 0)
			break;
		if ((i < 0) && (errno != EINTR)) {
			buffer->last_err = SZBE_SYSERR;
			free(buf);
			close(fd);
			return false;
		}
		if (i > 0)
			to_read -= i;
	}
	close(fd);

	assert(!(to_read % sizeof(SZB_FILE_TYPE)));

	this->fixed_probes_count = this->probes_from_file = max_probes - (to_read / sizeof(SZB_FILE_TYPE));

	this->AllocateDataMemory();

	// konwersja do SZBASE_TYPE
	double pw = pow(10.0, param->GetPrec());
	for (i = 0; i < this->probes_from_file; i++) {
		if (SZB_FILE_NODATA == buf[i]) {
			this->data[i] = SZB_NODATA;
		} else {
			if (first_data_probe_index < 0)
				first_data_probe_index = i;
			this->data[i] = SZBASE_TYPE (((double) szbfile_endian(buf[i])) / pw);
			this->last_data_probe_index = i;
		}
	}

	for (; i < this->max_probes; i++)
		this->data[i] = SZB_NODATA;

	free(buf);

	return true;
}

/** Reloads block if size of block's file has changed. If file size is
 * the same as when data was loaded to block, nothing happens.
 * @param buffer pointer to szbase buffer
 * @param block pointer to block to reload
 * @return -1 on error (check buffer->last_err), otherwise 0
 */
void RealDatablock::Refresh() {
	if (this->fixed_probes_count == this->max_probes)
		return;

	sz_log(DATABLOCK_REFRESH_LOG_LEVEL, "RealDatablock::Refresh() '%ls'", this->GetBlockRelativePath().c_str());

	time_t updateTime = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	if (this->last_update_time == updateTime)
		return;

	this->last_update_time = updateTime;

	if (!fs::exists(GetBlockFullPath())) {
		if (this->first_data_probe_index < 0) { //this block is empty - update fixed probes
			int tmp = szb_probeind(this->last_update_time < this->GetLastDataProbeIdx() ? this->last_update_time
					: this->GetLastDataProbeIdx());
			assert(tmp >= this->fixed_probes_count);
			this->fixed_probes_count = this->fixed_probes_count > tmp ? this->fixed_probes_count : tmp;
		}
		return;
	}

	bool memallocated = false;
	if (this->data == NULL) { //This is not an empty block any more
		this->AllocateDataMemory();
		memallocated = true;
	}

	size_t size = fs::file_size(GetBlockFullPath());
	assert(!(size % sizeof(SZB_FILE_TYPE)));

	/* check if file size changed */
	int i = size / sizeof(SZB_FILE_TYPE);
	assert(i >= this->probes_from_file);

	if (i <= this->probes_from_file)
		return;

	/* load data */
#ifdef MING32
	int fd = open(SC::S2A(GetBlockFullPath()).c_str(), O_RDONLY | O_BINARY);
#else
	int fd = open(SC::S2A(GetBlockFullPath()).c_str(), O_RDONLY);
#endif

	if (fd < 0) {
		this->buffer->last_err = SZBE_SYSERR;
		NOT_INITIALIZED;
	}
	/* we load only new data */
	int pos = this->probes_from_file * sizeof(SZB_FILE_TYPE);
	if (lseek(fd, pos, SEEK_SET) != pos) {
		sz_log(1, "szb_reload_block: lseek() failed");
		close(fd);
		NOT_INITIALIZED;
	}

	int r = size - pos;
	int to_read = r;
	int prev_probes_c = this->probes_from_file;

	// for conversion to SZBASE_TYPE
	SZB_FILE_TYPE * buf = (SZB_FILE_TYPE *) malloc(sizeof(SZB_FILE_TYPE) * r);
	if (NULL == buf) {
		this->buffer->last_err = SZBE_OUTMEM;
		NOT_INITIALIZED;
	}

	while (r > 0) {
		i = read(fd, (char *)buf + (to_read - r), r);
		if (i == 0)
			break;
		if ((i < 0) && (errno != EINTR)) {
			this->buffer->last_err = SZBE_SYSERR;
			free(buf);
			close(fd);
			return;
		}
		if (i > 0) {
			r -= i;
			pos += i;
			this->probes_from_file += i / sizeof(SZB_FILE_TYPE);
		}
	}
	close(fd);

	// conversion to SZBASE_TYPE
	to_read = to_read / sizeof(SZB_FILE_TYPE);
	double pw = pow(10, this->param->GetPrec());
	for (i = 0; i < to_read; i++) {
		if (SZB_FILE_NODATA == buf[i])
			this->data[prev_probes_c + i] = SZB_NODATA;
		else {
			this->data[prev_probes_c + i] = SZBASE_TYPE (static_cast<double>(buf[i]) / pw);
			last_data_probe_index = prev_probes_c + i;
		}
	}
	free(buf);

	if (this->last_update_time > this->GetEndTime())
		this->fixed_probes_count = max_probes;
	else if (this->last_update_time < this->GetStartTime())
		this->fixed_probes_count = this->probes_from_file;
	else {
		int tmp = szb_probeind(this->last_update_time) + 1;
		this->fixed_probes_count = this->probes_from_file < tmp ? this->probes_from_file : tmp;
	}

	if (memallocated) {
		for (int i = this->fixed_probes_count; i < this->max_probes; i++)
			data[i] = SZB_NODATA;
	}

	return;
}

/** Search first available probe in given period for parameters saved in base.
 * @param buffer cache buffer
 * @param param parameter
 * @param start start time of search
 * @param end time of search
 * @param direction direction of search
 * @return time of first available probe, -1 for error
 */
time_t szb_real_search_data(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction) {
#ifdef KDEBUG
	sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: %s, s: %ld, e: %ld, d: %d",
			param->GetName(), start, end, direction);
#endif
	time_t t;

	if ((start >= 0) && !IS_SZB_NODATA(szb_get_data(buffer, param, start))) {
#ifdef KDEBUG
		sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: return: %ld", start);
#endif
		return start;
	}
	if (direction == 0) {
#ifdef KDEBUG
		sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: return: -1");
#endif
		return (time_t) (-1);
	}

	if (direction < 0) {
		if (end < 0)
			end = szb_search_first(buffer, param);
		if (end < 0) {
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: return: -1");
#endif
			return -1;
		}

		time_t last_time = szb_search_last(buffer, param);
		if (start < 0 || start > last_time)
			start = last_time; 
		assert(start >= 0);
		if (start < end) {
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: return: -1");
#endif
			return -1;
		}
	}
	else {
		assert(direction > 0);
		if (start < 0)
			start = szb_search_first(buffer, param);
		if (start < 0) {
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: return: -1");
#endif
			return -1;
		}
		if (end < 0)
			end = szb_search_last(buffer, param);
		if (end < 0) {
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: return: -1");
#endif
			return -1;
		}
		if (start> end) {
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: return: -1");
#endif
			return -1;
		}
	}

#ifdef KDEBUG
	sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: new start: %ld, naw end: %ld", start, end);
#endif

	if (direction < 0) {
		/* search for data in left direction */
		int year = 0, month = 0;
		int new_year, new_month;
		int index;

		szb_time2my(start, &year, &month);
#ifdef KDEBUG
		sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: start y: %d, m: %d", year, month);
#endif
		szb_datablock_t *block = NULL;
		const SZBASE_TYPE * data = NULL;

		for (t = start; t >= end; t -= SZBASE_DATA_SPAN) {
			/* check if block exists */
			szb_time2my(t, &new_year, &new_month);
			if (NULL == block || new_month != month || new_year != year) {
				block = szb_get_datablock(buffer, param, new_year, new_month);
					if (NULL != block)
						data = block->GetData();
				if (NULL == block || block->GetFirstDataProbeIdx() < 0) {
					/* jump to the begining of the block, so after next iteration
					 * we are at the end of previous */
					t = probe2time(0, new_year, new_month);
					continue;
				}
				year = new_year;
				month = new_month;
			}
			/* check for data at index */
			index = szb_probeind(t);
			assert(index >= 0);

			assert(block->GetFirstDataProbeIdx() >= 0);
			assert(!IS_SZB_NODATA(data[block->GetFirstDataProbeIdx()]));
			assert(!IS_SZB_NODATA(data[block->GetLastDataProbeIdx()]));

			if(index> block->GetLastDataProbeIdx()) { //If index is after last data jump to last data in next iteration
				t = probe2time(block->GetLastDataProbeIdx() + 1, new_year, new_month);
				continue;
			}
			if(index < block->GetFirstDataProbeIdx()) { //If index is before first data jump to the previous block in next iteration
				t = probe2time(0, new_year, new_month);
				continue;
			}

			if ( !IS_SZB_NODATA(data[index]) ) {
#ifdef KDEBUG
				sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: index: %d, return: %ld", index, t);
#endif
				return t;
			}
		}
#ifdef KDEBUG
		sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: return: -1");
#endif
		return -1;
	}

	int year = 0, month = 0, index = 0;
	int new_year = 0, new_month = 0;
	szb_datablock_t *block = NULL;
	const SZBASE_TYPE * data = NULL;

	szb_time2my(start, &year, &month);

#ifdef KDEBUG
	sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: start y: %d, m: %d", year, month);
#endif

	for (t = start; t <= end; t += SZBASE_DATA_SPAN) {
		szb_time2my(t, &new_year, &new_month);
		if (NULL == block || new_month != month || new_year != year) {
			block = szb_get_datablock(buffer, param, new_year, new_month);
			if (NULL != block)
			data = block->GetData();
			if (NULL == block || block->GetFirstDataProbeIdx() < 0) {
				/* go to the begining of next block */
				t = probe2time(szb_probecnt(new_year, new_month) - 1, new_year, new_month);
				continue;
			}
			year = new_year;
			month = new_month;
		}

		index = szb_probeind(t);
		assert(index >= 0);

		assert(!IS_SZB_NODATA(data[block->GetFirstDataProbeIdx()]));
		assert(!IS_SZB_NODATA(data[block->GetLastDataProbeIdx()]));

		if(index> block->GetLastDataProbeIdx()) { //If index is after last data jump to the next block
			t = probe2time(block->max_probes - 1, new_year, new_month);
			continue;
		}
		if(index < block->GetFirstDataProbeIdx()) { //If index is before first data jump first data in next iteration
			t = probe2time(block->GetFirstDataProbeIdx() - 1, new_year, new_month);
			continue;
		}

		if (!IS_SZB_NODATA(block->GetData()[index])) {
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: index: %d, return: %ld", index, t);
#endif
			return t;
		}
	}
#ifdef KDEBUG
	sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_real_search_data: return: -1");
#endif
	return -1;
}

const SZBASE_TYPE * RealDatablock::GetData(bool refresh) {
	if (refresh)
		this->Refresh();
	return data == NULL ? empty_field : data;
}
;

bool RealDatablock::empty_field_initialized = false;
SZBASE_TYPE * RealDatablock::empty_field;
