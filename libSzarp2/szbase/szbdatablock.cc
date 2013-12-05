/* 
  SZARP: SCADA software 

*/
/* 
 * szbase - datablock
 * $Id$
 * <pawel@praterm.com.pl>
 */

#include <config.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

#include "szbbuf.h"
#include "szbname.h"
#include "szbfile.h"
#include "szbdate.h"
#include "szbhash.h"

#include "definabledatablock.h"
#include "realdatablock.h"

#include "szbbase.h"
#include "include/szarp_config.h"

#include <stdlib.h>
#include <sstream>
#include <iomanip>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "liblog.h"

#include "szbbuf.h"
#include "luadatablock.h"
#include "szbdefines.h"
#include "realdatablock.h"
#include "combineddatablock.h"

#include "boost/filesystem/path.hpp"

namespace fs = boost::filesystem;

const SZBASE_TYPE *
szb_datablock_t::GetData(bool refresh)
{
	if(refresh)
		this->Refresh();
	return data;
}

szb_datablock_t::szb_datablock_t(szb_buffer_t * b, TParam * p, int y, int m) :
		szb_block_t(b, p, probe2time(0, y, m), probe2time(szb_probecnt(y, m) - 1, y, m)),
		year(y), month(m), 
		max_probes(szb_probecnt(y, m)),
		initialized(true),
		data(NULL),
		first_data_probe_index(-1),
		last_data_probe_index(-1),
		last_update_time(-1),
		block_timestamp(-1)
{
	assert(buffer != NULL);
	assert(p != NULL);
	assert(month > 0);
	assert(year > 0);
	assert(max_probes > 0);
}

szb_datablock_t::~szb_datablock_t()
{
	if (data != NULL)
		FreeDataMemory();
}

std::wstring
szb_datablock_t::GetBlockRelativePath() {
	return GetBlockRelativePath(param, year, month);
}

std::wstring
szb_datablock_t::GetBlockRelativePath(TParam * param, int year, int month) {
	if ((month < 1) || (month > 12))
		return L"";

	if ((year < SZBASE_MIN_YEAR) || (year > SZBASE_MAX_YEAR))
		return L"";


	std::wstringstream wss;
	wss << std::setw(4) << std::setfill(L'0') << year 
		<< std::setw(2) << std::setfill(L'0') << month 
		<< L".szb";

	fs::wpath paramPath = fs::wpath(param->GetSzbaseName()) / wss.str();

#if BOOST_FILESYSTEM_VERSION == 3
	return paramPath.wstring();
#else
	return paramPath.string();
#endif
}

std::wstring
szb_datablock_t::GetBlockFullPath() {

	return GetBlockFullPath(buffer, param, year, month);

}

std::wstring
szb_datablock_t::GetBlockFullPath(szb_buffer_t* buffer, TParam * param, int year, int month) {
	
	std::wstring rp = GetBlockRelativePath(param, year, month);
	if (rp.empty())
		return L"";

	fs::wpath paramPath = fs::wpath(buffer->rootdir) / rp;
#if BOOST_FILESYSTEM_VERSION == 3
	return paramPath.wstring();
#else
	return paramPath.string();
#endif
}

void szb_datablock_t::AllocateDataMemory() { assert(!data); data = new SZBASE_TYPE[this->max_probes]; }

void szb_datablock_t::FreeDataMemory() { assert(data); delete[] data;};

SZB_BLOCK_TYPE szb_datablock_t::GetBlockType() {
	return MIN10_BLOCK;
}
