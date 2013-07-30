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
 * combineddatablock
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include <fstream>

#ifdef MINGW32
#undef int16_t
typedef short int               int16_t;
#endif

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/once.hpp"
#include "boost/thread/xtime.hpp"

#include "include/szarp_config.h"
#include "liblog.h"
#include "cacheabledatablock.h"
#include "conversion.h"

typedef boost::recursive_mutex::scoped_lock recursive_scoped_lock;

namespace fs = boost::filesystem;

bool CacheableDatablock::write_cache = false;

CacheableDatablock::CacheableDatablock(szb_buffer_t * b, TParam * p, int y, int m) : szb_datablock_t(b, p, y, m), cachepath(GetCacheFilePath(p, y, m))
{
	sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, "Creating cachable block for param: %ls, year: %d, month: %d, cachepath: %ls", param->GetName().c_str(), year, month, cachepath.c_str());
}

void CacheableDatablock::Cache()
{
	if (write_cache == false)
		return;

	if (!IsInitialized() || buffer->cachepoison)
		return;

	if (!IsCachable())
		return;
	
	if (cachepath.empty()) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, "CacheableDatablock::Cache: no cache file specified");
		return;
	}

	if (!CreateCacheDirectory())
		return;

	std::ofstream ofs(SC::S2A(cachepath).c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
	ofs.write( (char*) data, sizeof(SZBASE_TYPE)*fixed_probes_count);
	sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, "CacheableDatablock::Cache: %d probes written to cache file: '%ls'", fixed_probes_count, cachepath.c_str());
	ofs.close();

}

std::wstring CacheableDatablock::GetCacheRootDirPath(szb_buffer_t *buffer) {

	assert(buffer);
	assert(!buffer->prefix.empty());

	char *home = 
#ifdef MINGW32
		getenv("APPDATA");
#else
		getenv("HOME");
#endif
	if (home == NULL)
		return std::wstring();

	fs::wpath cachepath = SC::A2S(home);
	if (!fs::exists(cachepath))
		return std::wstring();

	cachepath = cachepath / L".szarp" / L"szbase" / L"cache" / fs::wpath(buffer->prefix);

	return cachepath.wstring();
}

std::wstring CacheableDatablock::GetCacheFilePath(TParam * p, int y, int m)
{
	std::wstring cache = GetCacheRootDirPath(buffer);

	if (cache.empty())
		return std::wstring();

	fs::wpath ret = fs::wpath(cache) / this->GetBlockRelativePath();

	return ret.wstring();
}

bool
CacheableDatablock::LoadFromCache()
{
	if (cachepath.empty()) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
				"CacheableDatablock::LoadFromCache: no cache file specified");
		return false;
	}

	int probes;
	time_t mod;
	if (!IsCacheFileValid(probes, &mod)) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
				"CacheableDatablock::LoadFromCache: no valid cache file: '%ls'",
				cachepath.c_str());
		return false;
	}

	this->block_timestamp = mod;

	std::ifstream ifs(SC::S2A(cachepath).c_str(), std::ios::binary | std::ios::in);

	ifs.read((char*) data, sizeof(SZBASE_TYPE) * probes);

	if(ifs.fail() && !ifs.eof()) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL,
				"CacheableDatablock::LoadFromCache: error reading file: '%ls' !",
				cachepath.c_str());
		return false;
	}

	assert(ifs.gcount() == (int) sizeof(SZBASE_TYPE) * probes);
	assert(probes <= this->max_probes);
	
	ifs.close();

	this->fixed_probes_count = probes;

	for(int i = this->fixed_probes_count; i < this->max_probes; i++)
		data[i] = SZB_NODATA;

	for(int i = 0; i < this->fixed_probes_count; i++) //find first data
		if(!IS_SZB_NODATA(this->data[i])) {
			this->first_data_probe_index = i;
			break;
		}

	if(this->first_data_probe_index >= 0)
		for(int i = this->fixed_probes_count - 1; i >= 0; i--) //find last data
			if(!IS_SZB_NODATA(this->data[i])) {
				this->last_data_probe_index = i;
				break;
			}
	
	sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
			"CacheableDatablock::LoadFromCache: loaded %d/%d probes from cache file: '%ls'", 
			this->fixed_probes_count, 
			this->max_probes,
			cachepath.c_str());
	assert(this->fixed_probes_count <= this->max_probes);

	return true;
}

bool
CacheableDatablock::IsCacheFileValid(int &probes, time_t *mdate)
{
	probes = 0;
	if (!fs::exists(cachepath)) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
				"CacheableDatablock::IsCacheFileValid: file does not exist: '%ls'", 
				this->cachepath.c_str());
		return false;
	}
	size_t size;
	try {
		size = fs::file_size(cachepath);
	} catch (fs::filesystem_error) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL,
				"CacheableDatablock::IsCacheFileValid: cannot retrive file szie for: '%ls'",
				this->cachepath.c_str());
		return false;
	}

	time_t mtime;
	try {
		mtime = fs::last_write_time(cachepath);
	} catch (fs::filesystem_error) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, "CacheableDatablock::IsCacheFileValid: cannot retrive modification date for: '%ls'", this->cachepath.c_str());
		return false;
	}

	if(mdate != NULL)
		*mdate = mtime;


	probes = size;
	if (probes % sizeof(SZBASE_TYPE) != 0 ) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
				"CacheableDatablock::IsCacheFileValid: cache file has invalid size: '%ls'",
				cachepath.c_str());
		return false;
	}

	if (mtime < buffer->configurationDate ) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
				"CacheableDatablock::IsCacheFileValid: cache file is older then params.xml: '%ls'",
				cachepath.c_str());
		return false;
	}

	probes /= sizeof(SZBASE_TYPE);

	fs::wpath tmp = buffer->rootdir;

	tmp = tmp / L".." / L"szbase_stamp";

	time_t bmt;
	try {
		bmt = fs::last_write_time(tmp);
	} catch (fs::filesystem_error) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, "CacheableDatablock::IsCacheFileValid: cannot retrive modification date of szbase_stamp");
		return true;
	}

	return mtime > bmt;
}

bool
CacheableDatablock::CreateCacheDirectory()
{
#ifdef MINGW32	
	if(!getenv("APPDATA"))
		return false;
#else
	if(!getenv("HOME"))
		return false;
#endif

	fs::wpath tmppath(cachepath);
	tmppath.remove_leaf();
	if(fs::exists(tmppath))
		return true;

	if(!fs::create_directories(tmppath)) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, "CacheableDatablock::CreateCacheDirectory: cannot create directory: %ls", cachepath.c_str());
		return false;
	}

	return true;
}

void 
CacheableDatablock::ClearCache(szb_buffer_t *buffer)
{
	buffer->cachepoison = true;
	boost::filesystem::remove_all(CacheableDatablock::GetCacheRootDirPath(buffer));
}

void 
CacheableDatablock::ResetCache(szb_buffer_t *buffer)
{
	buffer->cachepoison = false;
}

class RemoveBlockPredicate
{
	public:
		TParam *deleteParam;
		
		RemoveBlockPredicate(TParam *param) 
		{
			this->deleteParam = param;
		}
		
		bool operator()(CacheBlockEntry cbe)
		{
			if(cbe.block != NULL) {
				if(cbe.block->param == deleteParam) {
					return true;
				} else
 					return false;
			} else
				return true;
		}
		
};

void 
CacheableDatablock::ClearParamFromCache(szb_buffer_t *buffer, TParam *param)
{
	boost::filesystem::wpath tmp(CacheableDatablock::GetCacheRootDirPath(buffer));
	tmp /= param->GetSzbaseName();

	boost::filesystem::remove_all(tmp);
}

CacheableDatablock::~CacheableDatablock() {
	sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, "Destroying cachable block for param: %ls, year: %d, month: %d", param->GetName().c_str(), year, month);
	Cache();
}
