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
#include <config.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

#include "conversion.h"

#include "szbdefines.h"
#include "szbbuf.h"
#include "szbname.h"
#include "szbfile.h"
#include "szbdate.h"
#include "szbhash.h"

#include "szbbase.h"
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
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "liblog.h"

#include "szbbuf.h"
#include "luadatablock.h"
#include "szbdefines.h"
#include "realdatablock.h"
#include "combineddatablock.h"
#include "definabledatablock.h"
#include "loptdatablock.h"
#include "loptcalculate.h"
#include "proberconnection.h"
#include "szbsearch.h"
#include "luacalculate.h"

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"

namespace fs = boost::filesystem;

#if BOOST_FILESYSTEM_VERSION != 3
#define filesystem_error wfilesystem_error
#endif

#define szb_hfun szb_hfun2

#define uhashsize(n) ((ub4)1<<(n))
#define hashmask(n) (uhashsize(n)-1)

#define szb_hfun2(a, n) (hash(a, strlen((const char *) a), n) & hashmask(hashbits))

namespace {

template<typename OP> std::wstring find_one_param_file(const fs::wpath &paramPath, OP op) {

	std::wstring file;
	try {
#if BOOST_FILESYSTEM_VERSION == 3
		for( fs::directory_iterator i(paramPath); i != fs::directory_iterator(); ++i )
		{
			std::wstring l = i->path().filename().wstring();
#else
		for (fs::wdirectory_iterator i(paramPath); 
				i != fs::wdirectory_iterator(); 
				i++) {
			std::wstring l = i->path().leaf();
#endif
			if (is_szb_file_name(l) == false)
				continue;

			if (file.empty()) {
				file = l;
				continue;
			}
			
			if (op(l, file))
				file = l;
	
		}

	} catch (fs::filesystem_error &e) {
		file.clear();
	}

	return file;
}	

}

/** Searches for block in buffer. Does not load blocks, only searches within
 * already loaded blocks.
 * @param buffer buffer pointer, mey not be NULL
 * @param param name of parameter encoded by latin2szb
 * @param year year (4 digits)
 * @param month month (from 1 to 12)
 * @return pointer to block found, NULL if not found
 */
szb_datablock_t *
szb_find_block(szb_buffer_t * buffer, TParam * param, int year, int month)
{
    return buffer->FindDataBlock(param, year, month);
}


/** Returns pointer to block for given param, year and month. 
 * @param buffer pointer to szbase buffer context
 * @param param pointer to parameter
 * @param year year (4 digits)
 * @param month month (from 1 to 12)
 * @return pointer to data block, NULL if file for block does not exists
 * or couldn't be loaded, buffer->last_err is set to error code
 */
szb_datablock_t *
szb_get_datablock(szb_buffer_t * buffer, TParam * param, int year, int month)
{
	assert(NULL != buffer);

	szb_datablock_t *ret = szb_find_block(buffer, param, year, month);
	if (NULL != ret)
		return ret;

	try {
		if( param->IsDefinable() )
			param->PrepareDefinable();
	} catch( TCheckException& e ) {
		return NULL;
	}

    switch (param->GetType()) {
	case ParamType::REAL:
		ret = new RealDatablock(buffer, param, year, month);
		break;
	case ParamType::COMBINED:
		ret = new CombinedDatablock(buffer, param, year, month);
		break;
	case ParamType::DEFINABLE:
		ret = new DefinableDatablock(buffer, param, year, month);
	    break;
#ifndef NO_LUA
	case ParamType::LUA:
	    	if (param->GetFormulaType() == FormulaType::LUA_AV)
			return NULL;
		ret = create_lua_data_block(buffer, param, year, month);
	    break;
#endif

	default:
		fprintf(stderr,  "szb_calculate_block_default\n");
		ret = NULL;
		break;
	}

	if (NULL != ret) {
		buffer->AddBlock(ret);
		ret->FinishInitialization();

		if(!ret->IsInitialized())
		{
			buffer->DeleteBlock(ret);
			ret = NULL;
		}
	}

	return ret;
}

szb_datablock_t *
szb_get_datablock(szb_buffer_t * buffer, TParam * param, time_t time) {
	int year, month;
	szb_time2my(time, &year, &month);
	return szb_get_datablock(buffer, param, year, month);
}

szb_block_t *
szb_get_block(szb_buffer_t * buffer, TParam * param, time_t time, SZB_BLOCK_TYPE bt) {
	switch (bt) {
		case MIN10_BLOCK:
			return szb_get_datablock(buffer, param, time);
		case SEC10_BLOCK:
			return szb_get_probeblock(buffer, param, time);
		case UNUSED_BLOCK_TYPE:
			assert(false);
	}
	return NULL;	
}

szb_probeblock_t *
szb_get_probeblock(szb_buffer_t * buffer, TParam * param, time_t time){
	assert(NULL != buffer);

	time = szb_round_to_probe_block_start(time);

	szb_probeblock_t *ret = buffer->FindProbeBlock(param, time);
	if (ret == NULL) {
		ret = szb_create_probe_block(buffer, param, time);
		buffer->AddBlock(ret);
	}
	return ret;
}

time_t
szb_search_first(szb_buffer_t * buffer, TParam * param)
{
	int year, month;
	fs::wpath rootPath(buffer->rootdir);
	fs::wpath paramPath(rootPath / param->GetSzbaseName());

	std::wstring first = find_one_param_file(paramPath, std::less<std::wstring>());
	if (first.empty())
		return -1;

	szb_path2date(first, &year, &month);
	if ((year <= 0) || (month <= 0))
		return -1;
	return probe2time(0, year, month);
}

time_t
szb_search_last(szb_buffer_t * buffer, TParam * param)
{
	switch(param->GetType()) {
		case ParamType::REAL:
			{
				int year, month;
				
				fs::wpath rootPath(buffer->rootdir);
				fs::wpath paramPath(rootPath / param->GetSzbaseName());

				std::wstring last = find_one_param_file(paramPath, std::greater<std::wstring>());
				if (last.empty()) {
					return -1;
				}

				szb_path2date(last, &year, &month);

				if ((year <= 0) || (month <= 0)) {
					assert(false);
				}

				fs::wpath paramFilePath(paramPath / last);

				size_t size;
				try {
					size = fs::file_size(paramFilePath);
				} catch (fs::filesystem_error& e) {
					return -1;
				}

				return probe2time(size / 2 - 1, year, month);
			}
		case ParamType::COMBINED:
			{
				assert(param->GetNumParsInFormula() == 2);
				TParam ** cache = param->GetFormulaCache();

				// search for lsw param
				time_t t = szb_search_last(buffer, cache[0]);
				if (-1 == t)
					return -1;

				// if data not found search for msw param
				if (IS_SZB_NODATA(szb_get_data(buffer, param, t)))
					t = szb_search_last(buffer, cache[1]);
				return t;
			}
		case ParamType::DEFINABLE:
			{
				TParam ** cache = param->GetFormulaCache();
				if (param->GetNumParsInFormula() == 0 || param->IsNUsed())
					return buffer->GetMeanerDate();
				
				time_t t = buffer->GetMeanerDate();
				for(int i = 0; i < param->GetNumParsInFormula(); i++)
					t = std::min(t, szb_search_last(buffer, cache[i]));
				return t;
			}
#ifndef NO_LUA
		case ParamType::LUA:
			{
				time_t last_av_date = buffer->GetMeanerDate();
				return last_av_date + param->GetLuaEndOffset();
			}
#endif
		default:
			return -1;
	}

}

time_t
szb_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle, const szb_search_condition& condition) {
	
	if (buffer->PrepareConnection() == false)
		return -1;

	switch (param->GetType()) {
		case ParamType::REAL:
			return szb_real_search_probe(buffer, param, start, end, direction, c_handle, condition);
	    
		case ParamType::COMBINED:
			return szb_combined_search_probe(buffer, param, start, end, direction, c_handle, condition);
	    
		case ParamType::DEFINABLE:
			return szb_definable_search_probe(buffer, param, start, end, direction, c_handle, condition);

#ifndef NO_LUA
		case ParamType::LUA:
			return szb_lua_search_probe(buffer, param, start, end, direction, c_handle, condition);
#endif
		default:
			return -1;
	}
	return -1;
}

time_t
szb_search_data(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SZARP_PROBE_TYPE probe, SzbCancelHandle * c_handle, const szb_search_condition& condition)
{
    switch(param->GetType()) {
	case ParamType::REAL:
	    return szb_real_search_data(buffer, param, start, end, direction, condition);
	    
	case ParamType::COMBINED:
	    return szb_combined_search_data(buffer, param, start, end, direction, condition);
	    
	case ParamType::DEFINABLE:
	    return szb_definable_search_data(buffer, param, start, end, direction, condition);

#ifndef NO_LUA
	case ParamType::LUA:
	   return szb_lua_search_data(buffer, param, start, end, direction, condition);
#endif
	default:
	    return -1;
    }
}

bool szb_nan_search_condition::operator()(const double& v) const {
	return !IS_SZB_NODATA(v);
}

time_t
szb_search(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SZARP_PROBE_TYPE probe, SzbCancelHandle * c_handle, const szb_search_condition& condition)
{
#ifndef NO_LUA
	if (param->GetType() == ParamType::LUA && param->GetFormulaType() == FormulaType::LUA_AV)
		return szb_lua_search_by_value(buffer, param, probe, start, end, direction, condition);
#endif
	if (probe == PT_SEC10)
		return szb_search_probe(buffer, param, start, end, direction, c_handle, condition);
	else
		return szb_search_data(buffer, param, start, end, direction, probe, c_handle, condition);
}


SZBASE_TYPE
szb_get_data(szb_buffer_t * buffer, TParam * param, time_t time)
{
#ifdef KDEBUG
	sz_log(9, "szb_get_data: %s (%ld) [%s]",
		param->GetName(), (unsigned long int) time, ctime(&time));
#endif

	if (param->IsConst()) {
		if (time >= buffer->first_av_date && time <= szb_search_last(buffer, param))
			return param->GetConstValue();
		else
			return SZB_NODATA;
	}

	int year, month, index;
	szb_datablock_t *block;

	szb_time2my(time, &year, &month);
	
	block = szb_get_datablock(buffer, param, year, month);

	index = szb_probeind(time);
	assert(index >= 0);

	return block != NULL ? block->GetData()[index] : SZB_NODATA;
}

#define NOT_FIXED if(is_fixed) *is_fixed = false;

void 
szb_get_avg_probe(szb_buffer_t * buffer, TParam * param,
	time_t start_time, time_t end_time, double &psum, int &pcount, SZARP_PROBE_TYPE probe_type, bool *is_fixed, double *first, double *last) {

	szb_probeblock_t* b;
	int probe;

	while (start_time < end_time) {
		/* check current block */
		time_t block_start = szb_round_to_probe_block_start(start_time);
		probe = (start_time - block_start) / SZBASE_PROBE_SPAN;
		b = szb_get_probeblock(buffer, param, block_start);
		if (b != NULL) {
			const SZBASE_TYPE * data = b->GetData();

			if (b->GetFixedProbesCount() < b->probes_per_block
					&& time_t(b->GetFixedProbesCount() * SZBASE_PROBE_SPAN + b->GetStartTime()) < end_time)
				NOT_FIXED;

			time_t t = start_time;
			/* scan block for values */
			for (int i = probe; t < end_time && i < SZBASE_PROBES_IN_BLOCK; i++, t += SZBASE_PROBE_SPAN) {
				if (!IS_SZB_NODATA(data[i])) {
					psum += data[i];
					pcount++;
					if (last)
						*last = data[i];
					if (pcount == 1 && first)
						*first = data[i];
				}
			}
		}
		start_time += szb_probeblock_t::probes_per_block * SZBASE_PROBE_SPAN;
	}

}

void
szb_get_avg_data(szb_buffer_t * buffer, TParam * param,
	time_t start_time, time_t end_time, double& psum, int &pcount, SZARP_PROBE_TYPE probe_type, bool *is_fixed, double* first, double*last)
{

	szb_datablock_t *b;
	int year, month;
	int probe, max, i;
	time_t t = start_time;

	if (param->IsConst()) {

		time_t lad = szb_search_last(buffer, param);

		if (end_time > lad)
			end_time = lad;

		if (start_time < buffer->first_av_date)
			start_time = buffer->first_av_date;
	
		if (start_time <= end_time) {
			pcount = (end_time - start_time) / SZBASE_DATA_SPAN;
			psum = param->GetConstValue() * pcount;
			if (first)
				*first = param->GetConstValue();
			if (last)
				*last = param->GetConstValue();
			return;
		} else {
			if (first)
				*first = SZB_NODATA;
			if (last)
				*last = SZB_NODATA;
			psum = SZB_NODATA;
			pcount = 0;
			return;
		}

    }

    while (t < end_time) {
		/* check current block */
		szb_time2my(t, &year, &month);
		probe = szb_probeind(t);
		max = szb_probecnt(year, month);
		b = szb_get_datablock(buffer, param, year, month);
		if (b != NULL) {
			const SZBASE_TYPE * data = b->GetData();

			if (b->GetFixedProbesCount() < b->max_probes 
				&& end_time > probe2time(b->GetFixedProbesCount() - 1, b->year, b->month))
				NOT_FIXED;

			/* scan block for values */
			for (i = probe; (i <= b->GetLastDataProbeIdx()) && (t < end_time); i++, t += SZBASE_DATA_SPAN){
				if (!IS_SZB_NODATA(data[i])) {
					psum += data[i];
					pcount++;
					if (last)
						*last = data[i];
					if (pcount == 1 && first)
						*first = data[i];
				}
			}
		}
		/* set t to the begining of next month */
		t = probe2time(0, year, month) + max * SZBASE_DATA_SPAN;
	}
}

SZBASE_TYPE
szb_get_avg(szb_buffer_t * buffer, TParam * param,
	time_t start_time, time_t end_time, double * psum, int *pcount, SZARP_PROBE_TYPE probe_type, bool *is_fixed, double *first, double *last) {

	if (is_fixed)
		*is_fixed = true;

	double sum = 0.0;
	int count = 0;

#ifndef NO_LUA
	if (param->GetType() == ParamType::LUA && param->GetFormulaType() == FormulaType::LUA_AV) {
		bool f = true;
		SZBASE_TYPE tmp = szb_lua_get_avg(buffer, param, start_time, end_time, psum, pcount, probe_type, f);
		if (is_fixed)
			*is_fixed = f;
		return tmp;
	}
#endif

	if (probe_type == PT_SEC10)
		szb_get_avg_probe(buffer, param, start_time, end_time, sum, count, probe_type, is_fixed, first, last);
	else
		szb_get_avg_data(buffer, param, start_time, end_time, sum, count, probe_type, is_fixed, first, last);

	if (count <= 0) {
		if (nullptr != psum)
			*psum = SZB_NODATA;
		if (nullptr != pcount)
			*pcount = 0;
		if (first)
			*first = SZB_NODATA;
		if (last)
			*last = SZB_NODATA;
		return SZB_NODATA;
	}

	if (nullptr != psum) {
		*psum = sum;
		/* Workaround: getting proper sum in 30min range of szb data */
		if (probe_type == PT_SEC10)
			*psum /= 60;
	} if (nullptr != pcount) {
		*pcount = count;
	}
	return sum / count;
}


SZBASE_TYPE
szb_get_probe(szb_buffer_t * buffer, TParam * param,
	time_t t, SZARP_PROBE_TYPE probe, int custom_probe)
{
    time_t s = szb_round_time(t, probe, custom_probe);
    time_t e = szb_move_time(s, 1, probe, custom_probe);

    return szb_get_avg(buffer, param,
		s , e ,
		NULL,
		NULL,
		probe);
}

#undef NOT_FIXED

/** Gets last available date for base.
 * @param buffer pointer to buffer
 * @return last available date
 */
time_t
szb_get_last_av_date(szb_buffer_t * buffer)
{
	// get current time
	time_t t = szb_round_time(time(NULL), PT_MIN10, 0);

	time_t last_av_date = buffer->GetMeanerDate();
	if (last_av_date < 0) {
		// search through all params
		TParam * param = buffer->first_param;
	
		while (param) {
			t = szb_search(buffer, param, t, -1, -1);
			if (t > last_av_date)
				last_av_date = t;
			param = param->GetNext();
		}
	}
	if (last_av_date < 0)
		time(&last_av_date);
#ifdef KDEBUG
	sz_log(9, "L: szb_get_last_av_date: %ld", last_av_date);
#endif

	return last_av_date;
}

szb_buffer_t *
szb_create_buffer(Szbase *szbase, const std::wstring &directory, int num, TSzarpConfig * ipk)
{
	assert(ipk != NULL);

	if (num < 1)
		return NULL;

	szb_buffer_t * ret = new szb_buffer_t(num);
	if (ret == NULL)
		return NULL;

	ret->szbase = szbase;

	fs::wpath rootpath(directory);
#if BOOST_FILESYSTEM_VERSION == 3
	ret->rootdir = rootpath.wstring();
#else
	ret->rootdir = rootpath.string();
#endif

#if BOOST_FILESYSTEM_VERSION == 3
	ret->prefix = std::next(rootpath.end(),-2)->wstring();
#else
	fs::wpath tmppath(ret->rootdir);
	tmppath.remove_leaf().remove_leaf();
	ret->prefix = tmppath.leaf();
#endif

	ret->first_av_date = -1;
	ret->first_param = ipk->GetFirstParam();

	TParam * p = new TParam(NULL);
	p->Configure(L"Status:Meaner3:program uruchomiony",
			L"", L"", L"", NULL, 0, -1, 1);
	ret->meaner3_param = p;

	TParam *param = ipk->GetFirstParam();
	while (ret->first_av_date < 0 && (param = param->GetNextGlobal()))
		ret->first_av_date = szb_search_first(ret, param);

	time_t meaner_start = szb_search_first(ret, ret->meaner3_param);
	if (meaner_start > 0 && meaner_start < ret->first_av_date)
		ret->first_av_date = meaner_start;

	szb_time2my(ret->first_av_date, &(ret->first_av_year), &(ret->first_av_month));

	rootpath /= p->GetSzbaseName();
	// prepare mpath for fast path creation
#if BOOST_FILESYSTEM_VERSION == 3
	ret->meaner3_path += rootpath.wstring();
#else
	ret->meaner3_path += rootpath.string();
#endif

	ret->last_err = SZBE_OK;

	ret->last_query_id = -1;

	ret->configurationDate = ret->GetConfigurationDate();

	return ret;
}

void
szb_reset_buffer(szb_buffer_t * buffer)
{
    if (buffer == NULL)
	return;
    buffer->Reset();
}

int
szb_free_buffer(szb_buffer_t * buffer)
{
	if (NULL == buffer)
		return 0;

	delete buffer;
	return 0;
}

/** Locks buffer. Prevents removing block from cache.
 * @param buffer pointer to buffer.
 */
void
szb_lock_buffer(szb_buffer_t * buffer)
{
    buffer->Lock();
}

/** Unlocks buffer. Allows removing blocks from cache.
 * Removes old block if number of block is greater
 * then max_blocks.
 * @param buffer pointer to buffer.
 */
void
szb_unlock_buffer(szb_buffer_t * buffer)
{
	buffer->Unlock();
}

time_t
szb_definable_meaner_last(szb_buffer_t * buffer)
{
	if (buffer->last_query_id == buffer->szbase->GetQueryId())
		return buffer->last_av_date;

	int year, month;

	fs::wpath meaner3path(buffer->meaner3_path);

	std::wstring last = find_one_param_file(meaner3path, std::greater<std::wstring>());
	if (last.empty())
		return -1;

	szb_path2date(last, &year, &month);
	if ((year <= 0) || (month <= 0))
		return -1;

	size_t size;
	try {
		size = fs::file_size(meaner3path / last);
	} catch (fs::filesystem_error& e) {
		return -1;
	}


	time_t t;
#ifdef KDEBUG
	t = probe2time(size / 2 - 1, year, month);
	sz_log(10, "szb_definable_meaner_last: size: %d, t = %ld", st.st_size, t);
	return t;
#else
	t = probe2time(size / 2 - 1, year, month);
#endif

	buffer->last_query_id = buffer->szbase->GetQueryId();
	buffer->last_av_date = t;
	
	return t;

}

szb_buffer_str::szb_buffer_str(int size): newest_block(NULL), oldest_block(NULL),
	blocks_c(0), max_blocks(size), locked(0), prober_connection(NULL), cachepoison(false)
{
}

szb_buffer_str::~szb_buffer_str()
{
	this->freeBlocks();
#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
	for (size_t i = 0; i < optimized_params.size(); i++) {
		delete optimized_params[i]->GetLuaExecParam();
		optimized_params[i]->SetLuaExecParam(NULL);
	}
#endif
#endif
}

szb_datablock_t *
szb_buffer_str::FindDataBlock(TParam * param, int year, int month)
{
	
	szb_datablock_t * block = datastorage[BufferKey(param, probe2time(0, year, month))];

	if (block != NULL) {
		block->MarkAsUsed();
		return block;
	}
	return NULL;

}

szb_probeblock_t *
szb_buffer_str::FindProbeBlock(TParam * param, time_t start_time)
{
	
	szb_probeblock_t * block = probestorage[BufferKey(param, start_time)];

	if (block != NULL) {
		block->MarkAsUsed();
		return block;
	}

	return NULL;

}

time_t
szb_buffer_str::GetConfigurationDate() {
	fs::wpath configPath(this->rootdir);
	configPath = configPath / L".." / L"config" / L"params.xml";

	try {
		return fs::last_write_time(configPath);
	} catch (fs::filesystem_error& e) {
		return -1;
	}

}

std::wstring szb_buffer_str::GetConfigurationFilePath() {
	fs::wpath configPath(this->rootdir);
	configPath = configPath / L".." / L"config" / L"params.xml";

	std::wstring ret;
	try {
		if (fs::exists(configPath))
#if BOOST_FILESYSTEM_VERSION == 3
			ret = configPath.wstring();
#else
			ret = configPath.string();
#endif
	} catch (fs::filesystem_error& e) {
	}

	return ret;
}

std::wstring szb_buffer_str::GetSzbaseStampFilePath() {
	fs::wpath path(this->rootdir);
	path = path / L".." / L"szbase_stamp";

	std::wstring ret;

	try {
		if (fs::exists(path))
#if BOOST_FILESYSTEM_VERSION == 3
			ret = path.wstring();
#else
			ret = path.string();
#endif
	} catch (fs::filesystem_error& e) {
	}

	return ret;
}

void 
szb_buffer_str::ClearCache()
{
	bool cachepoison_ = cachepoison;
	cachepoison = true;
	CacheableDatablock::ClearCache(this);
	this->Reset();
	cachepoison = cachepoison_;
}

void 
szb_buffer_str::ClearParamFromCache(TParam* param)
{
	bool cachepoison_ = cachepoison;
	cachepoison = true;
	CacheableDatablock::ClearParamFromCache(this, param);
	while (this->paramindex[param] != NULL) {
		szb_block_t* tmp = this->paramindex[param]->block;
		DeleteBlock(tmp);
	}
	cachepoison = cachepoison_;
}

void 
szb_buffer_str::Reset()
{
	bool cachepoison_ = cachepoison;
	cachepoison = true;
	freeBlocks();
	cachepoison = cachepoison_;
	assert(this->blocks_c == 0);
	configurationDate = this->GetConfigurationDate();
	last_err = SZBE_OK;

}

void
szb_buffer_str::freeBlocks()
{
	while (datastorage.begin() != datastorage.end()) {
		delete datastorage.begin()->second;
		datastorage.erase(datastorage.begin());
	}

	while (probestorage.begin() != probestorage.end()) {
		delete probestorage.begin()->second;
		probestorage.erase(probestorage.begin());
	}
}

void
szb_buffer_str::DeleteBlock(szb_block_t* block)
{
	assert(block);

	BufferKey key(block->param, block->GetStartTime());
	switch (block->GetBlockType()) {
		case MIN10_BLOCK:
			assert(datastorage.erase(key) == 1);
			break;
		case SEC10_BLOCK:
			assert(probestorage.erase(key) == 1);
			break;
		default:
			assert(false);
			break;
	}

	delete block;

}

void
szb_buffer_str::AddBlock(szb_block_t* block)
{
	switch (block->GetBlockType()) {
		case MIN10_BLOCK:
			datastorage[BufferKey(block->param, block->start_time)] = dynamic_cast<szb_datablock_t*>(block);
			break;
		case SEC10_BLOCK:
			probestorage[BufferKey(block->param, block->start_time)] = dynamic_cast<szb_probeblock_t*>(block);
			break;
		default:
			assert(false);
			break;
	}
	block->locator = new BlockLocator(this, block);
}

void
szb_buffer_str::Lock()
{
	this->locked++;
}

void
szb_buffer_str::Unlock()
{
	this->locked--;
	assert(this->locked >= 0);

	if (this->locked)
		return;

	while (this->blocks_c > this->max_blocks) {
		szb_block_t * tmp = this->oldest_block->block;
		this->DeleteBlock(tmp);
	}

}

#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
void
szb_buffer_str::AddExecParam(TParam *param) {
	optimized_params.push_back(param);	
}

void
szb_buffer_str::RemoveExecParam(TParam *param) {
	std::vector<TParam*>::iterator e = std::remove(optimized_params.begin(), optimized_params.end(), param);
	optimized_params.erase(e, optimized_params.end());
	delete param->GetLuaExecParam();
	param->SetLuaExecParam(NULL);
}
#endif
#endif

bool
szb_buffer_str::PrepareConnection() {
	if (prober_connection)
		return true;
	std::wstring address, port;
	if (szbase->GetProberAddress(prefix, address, port)) {
		prober_connection = new ProberConnection(this, SC::S2A(address), SC::S2A(port));
		return true;
	} else {
		last_err = SZBE_CONN_ERROR;
		last_err_string = L"Connection with probes server not configured";
		return false;
	}
}

BlockLocator::BlockLocator(szb_buffer_t* buff, szb_block_t* b): block(b), buffer(buff),
	newer(NULL), older(NULL), next_same_param(NULL), prev_same_param(NULL)
{
	//insertion
	if(this->buffer->newest_block == NULL) {

		this->buffer->newest_block = this;
		this->buffer->oldest_block = this;

	} else {

		this->older = buff->newest_block;
		this->buffer->newest_block->newer = this;
		this->buffer->newest_block = this;
	}

	if (this->buffer->paramindex[this->block->param] == NULL) {

		this->buffer->paramindex[this->block->param] = this;

	} else {

		this->next_same_param = this->buffer->paramindex[this->block->param];
		this->buffer->paramindex[this->block->param]->prev_same_param = this;
		this->buffer->paramindex[this->block->param] = this;

	}

	this->buffer->blocks_c++;

	if (!this->buffer->locked) {
		while(this->buffer->blocks_c > this->buffer->max_blocks) {
			szb_block_t * tmp = this->buffer->oldest_block->block;
			this->buffer->DeleteBlock(tmp);
		}
	}

}

BlockLocator::~BlockLocator()
{
	this->buffer->blocks_c--;

	if (this->prev_same_param != NULL) {
		this->prev_same_param->next_same_param = this->next_same_param;
		if (this->next_same_param != NULL)
			this->next_same_param->prev_same_param = this->prev_same_param;
	} else {
		this->buffer->paramindex[this->block->param] = this->next_same_param;
		if (this->next_same_param != NULL)
			this->next_same_param->prev_same_param = NULL;
	}

	if (this == this->buffer->newest_block && this == this->buffer->oldest_block) {

		assert(this->older == NULL);
		assert(this->newer == NULL);
		this->buffer->newest_block = this->buffer->oldest_block = NULL;

	} else if(this == this->buffer->newest_block) {

		assert(this->newer == NULL);
		assert(this->older != NULL);
		this->buffer->newest_block = this->older;
		this->older->newer = NULL;

	} else if(this == this->buffer->oldest_block) {

		assert(this->older == NULL);
		assert(this->newer != NULL);
		this->buffer->oldest_block = this->newer;
		this->newer->older = NULL;
	} else { 
		assert(this->older != NULL);
		assert(this->newer != NULL);
		this->newer->older = this->older;
		this->older->newer = this->newer;
	}
}

void 
BlockLocator::Used()
{
	//We are the newest
	if (this == this->buffer->newest_block){
		assert(this->newer == NULL);
		return;
	}

	//We are the oldest
	if (this == this->buffer->oldest_block) {
		assert(this->older == NULL);
		assert(this->newer != NULL);
		this->buffer->oldest_block = this->newer;
		this->newer->older = NULL;
	} else {
		assert(this->older != NULL);
		assert(this->newer != NULL);
		this->newer->older = this->older;
		this->older->newer = this->newer;
	}

	//Insert at the begining
	this->newer = NULL;
	this->older = this->buffer->newest_block;
	this->older->newer = this;
	this->buffer->newest_block = this;

}
size_t 
TupleHasher::operator()(const BufferKey s) const
{
	size_t val = (size_t) s.get<0>();
	size_t val2 = s.get<1>();
	val ^= val2 << 14;
	return val;
}

#undef filesystem_error

