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

boost::recursive_mutex CacheableDatablock::cachequeue_mutex;
std::list<CacheBlockEntry> CacheableDatablock::cachequeue;

boost::once_flag flag = BOOST_ONCE_INIT;

bool CacheableDatablock::write_cache = false;

void CacheableDatablock::cachewriter()
{
	boost::xtime xt;
	boost::xtime_get(&xt, boost::TIME_UTC);
	xt.sec += 90;
	boost::thread::sleep(xt);

	while(1) {
		bool ismore = false;
		do{
			recursive_scoped_lock queuelock(cachequeue_mutex);
			if(CacheableDatablock::cachequeue.size() == 0)
				break;
			if(CacheableDatablock::cachequeue.front().block != NULL) {
				CacheableDatablock::cachequeue.front().block->Cache();
			}

			CacheableDatablock::cachequeue.pop_front();
			ismore = CacheableDatablock::cachequeue.size() > 0;
		} while(ismore);
		boost::xtime_get(&xt, boost::TIME_UTC);
		xt.sec += 90;
		boost::thread::sleep(xt);
	}

}

void CacheableDatablock::initcachewriter()
{
	if (write_cache)	
		boost::thread startthread(&CacheableDatablock::cachewriter);
}

CacheableDatablock::CacheableDatablock(szb_buffer_t * b, TParam * p, int y, int m) : szb_datablock_t(b, p, y, m), cachepath(GetCacheFilePath(p, y, m)), cached(false)
{

	boost::call_once(&initcachewriter, flag);

	recursive_scoped_lock queuelock(cachequeue_mutex);

	CacheBlockEntry e(this);
	CacheableDatablock::cachequeue.push_back(e);
	this->entry = &cachequeue.back();

}

void CacheableDatablock::Cache()
{
	recursive_scoped_lock queuelock(cachequeue_mutex);

	if(this->cached)
		return;

	assert(this->entry->block == this);

	this->entry->block = NULL;
	this->entry = NULL;
	this->cached = true;

	if(!this->IsInitialized() || this->buffer->cachepoison)
		return;

	if(!this->IsCachable())
		return;
	
	if(cachepath.empty()) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, "CacheableDatablock::Cache: no cache file specified");
		return;
	}

	if(!CreateCacheDirectory())
		return;

	std::ofstream ofs(SC::S2A(cachepath).c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
	ofs.write( (char*) data, sizeof(SZBASE_TYPE)*fixedProbesCount);
	sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, "CacheableDatablock::Cache: %d probes written to cache file: '%ls'", this->fixedProbesCount, cachepath.c_str());
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

	//11.II.2008 - removes old cache dir, remove me someday
	boost::filesystem::remove_all( cachepath / L".szbase" );

	cachepath = cachepath / L".szarp" / L"szbase" / L"cache" / fs::wpath(buffer->prefix);

	return cachepath.string();
}

std::wstring CacheableDatablock::GetCacheFilePath(TParam * p, int y, int m)
{
	std::wstring cache = GetCacheRootDirPath(buffer);

	if (cache.empty())
		return std::wstring();

	fs::wpath ret = fs::wpath(cache) / this->GetBlockRelativePath();

	return ret.string();
}

bool
CacheableDatablock::LoadFromCache()
{
	if(cachepath.empty()) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
				"CacheableDatablock::LoadFromCache: no cache file specified");
		return false;
	}

	int probes;
	time_t mod;
	if(!IsCacheFileValid(probes, &mod)) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
				"CacheableDatablock::LoadFromCache: no valid cache file: '%ls'",
				cachepath.c_str());
		return false;
	}

	this->block_timestamp = mod;
	assert(mod<=time(NULL));

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

	this->fixedProbesCount = probes;

	for(int i = this->fixedProbesCount; i < this->max_probes; i++)
		data[i] = SZB_NODATA;

	for(int i = 0; i < this->fixedProbesCount; i++) //find first data
		if(!IS_SZB_NODATA(this->data[i])) {
			this->firstDataProbeIdx = i;
			break;
		}

	if(this->firstDataProbeIdx >= 0)
		for(int i = this->fixedProbesCount - 1; i >= 0; i--) //find last data
			if(!IS_SZB_NODATA(this->data[i])) {
				this->lastDataProbeIdx = i;
				break;
			}
	
	assert(!IS_SZB_NODATA(this->data[this->GetFirstDataProbeIdx()]));
	assert(!IS_SZB_NODATA(this->data[this->GetLastDataProbeIdx()]));

	sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
			"CacheableDatablock::LoadFromCache: loaded %d/%d probes from cache file: '%ls'", 
			this->fixedProbesCount, 
			this->max_probes,
			cachepath.c_str());

	assert(this->fixedProbesCount <= this->max_probes);

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
	} catch (fs::wfilesystem_error) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL,
				"CacheableDatablock::IsCacheFileValid: cannot retrive file szie for: '%ls'",
				this->cachepath.c_str());
		return false;
	}

	time_t mtime;
	try {
		mtime = fs::last_write_time(cachepath);
	} catch (fs::wfilesystem_error) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, "CacheableDatablock::IsCacheFileValid: cannot retrive modification date for: '%ls'", this->cachepath.c_str());
		return false;
	}

	if(mdate != NULL)
		*mdate = mtime;


	probes = size;
	if( probes % sizeof(SZBASE_TYPE) != 0 ) {
		sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
				"CacheableDatablock::IsCacheFileValid: cache file has invalid size: '%ls'",
				cachepath.c_str());
		return false;
	}

	if( mtime < buffer->configurationDate ) {
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
	} catch (fs::wfilesystem_error) {
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
	recursive_scoped_lock queuelock(CacheableDatablock::cachequeue_mutex);
	buffer->cachepoison = true;
	boost::filesystem::remove_all(CacheableDatablock::GetCacheRootDirPath(buffer));
}

void 
CacheableDatablock::ResetCache(szb_buffer_t *buffer)
{
	recursive_scoped_lock queuelock(CacheableDatablock::cachequeue_mutex);
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
					cbe.block->cached = true;
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
	recursive_scoped_lock queuelock(CacheableDatablock::cachequeue_mutex);

	cachequeue.remove_if(RemoveBlockPredicate(param));

	boost::filesystem::wpath tmp(CacheableDatablock::GetCacheRootDirPath(buffer));
	tmp /= param->GetSzbaseName();

	boost::filesystem::remove_all(CacheableDatablock::GetCacheRootDirPath(buffer));
}

