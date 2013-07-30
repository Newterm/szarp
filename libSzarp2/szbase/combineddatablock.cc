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

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

#include "include/szarp_config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "boost/filesystem/operations.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "liblog.h"

#include "combineddatablock.h"
#include "realdatablock.h"
#include "conversion.h"
#include "szbsearch.h"



namespace fs = boost::filesystem;

/** Loads data blok for combined parameter.
 * @param buffer cache buffer
 * @param param parameter
 * @param year year (4 digits)
 * @param month month (1 - 12)
 * @return pointer to loaded block, NULL for error
 * (check buffer->last_err for error code)
 */
CombinedDatablock::CombinedDatablock(szb_buffer_t * b, TParam * p, int y, int m): szb_datablock_t(b, p, y, m)
{
#ifdef KDEBUG
	sz_log(DATABLOCK_CREATION_LOG_LEVEL, "D: CombinedDatablock::CombinedDatablock(%ls, %d.%d)", param->GetName().c_str(), year, month);
#endif

	this->last_update_time = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	this->AllocateDataMemory();

	int probes_count = szb_probecnt(year, month);
	int bytes_count =  probes_count * sizeof(SZB_FILE_TYPE);
	assert(bytes_count > 0);

	TParam ** p_cache = param->GetFormulaCache();

	//Most Significant Word
	std::wstring tmppath = szb_datablock_t::GetBlockFullPath(buffer, p_cache[0], year, month);
	if (tmppath.empty())
		NOT_INITIALIZED;

	SZB_FILE_TYPE * buf_msw = new SZB_FILE_TYPE[bytes_count];
	if (NULL == buf_msw) {
		buffer->last_err = SZBE_OUTMEM;
		NOT_INITIALIZED;
	}

	szb_file_exists(tmppath.c_str(), &this->block_timestamp);
	int msw_readed = szb_load_data(buffer, tmppath.c_str(), buf_msw, 0, bytes_count);

	if (-1 == msw_readed) {
		delete [] buf_msw;
		NOT_INITIALIZED;
	}

	// Less Significant Word
	tmppath = szb_datablock_t::GetBlockFullPath(buffer, p_cache[1], year, month);
	if (tmppath.empty())
		NOT_INITIALIZED;

	SZB_FILE_TYPE * buf_lsw =  new SZB_FILE_TYPE[bytes_count];
	if (NULL == buf_lsw) {
		delete [] buf_msw;
		buffer->last_err = SZBE_OUTMEM;
		NOT_INITIALIZED;
	}

	time_t mod;
	szb_file_exists(tmppath, &mod);
	this->block_timestamp = mod > this->block_timestamp ? mod : this->block_timestamp;
	int lsw_readed = szb_load_data(buffer, tmppath, buf_lsw, 0, bytes_count);

	if (-1 == lsw_readed) {
		delete [] buf_msw;
		delete [] buf_lsw;
		NOT_INITIALIZED;
	}

	if (msw_readed > lsw_readed) {
		// TODO - blad?
		msw_readed = lsw_readed;
	}

	this->fixed_probes_count =  msw_readed / sizeof(SZB_FILE_TYPE);

	// konwersja do SZB_TYPE
	double pw = pow(10.0, param->GetPrec());
	int i = 0;
	for (; i < this->fixed_probes_count; i++) {
		if (SZB_FILE_NODATA == buf_msw[i]) {
			this->data[i] = SZB_NODATA;
		} else {
			this->data[i] = (SZBASE_TYPE) ( (double)( 
				    int(szbfile_endian(buf_msw[i]) << 16)
				    	| (unsigned short)(szbfile_endian(buf_lsw[i]))
				    ) / pw );
		}

		if(!IS_SZB_NODATA(this->data[i])) {
			if(this->first_data_probe_index < 0)
				this->first_data_probe_index = i;
			this->last_data_probe_index = i;
		}

	}
	for (; i < probes_count; i++)
		this->data[i] = SZB_NODATA;

	delete [] buf_msw;
	delete [] buf_lsw;

#ifdef DDD
    printf("szb_load_block: block '%ls'\n", this->path.c_str());
#endif

	return;
}

/** Reloads combined block if size of data files has changed.
 * @param buffer szbase buffer
 * @param block block to reload
 * @return -1 on error (check buffer->last_err), otherwise 0
 */
void
CombinedDatablock::Refresh()
{
	// block is full - no more probes can be load
	if (this->fixed_probes_count == this->max_probes)
		return;

	time_t updatetime = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	if (this->last_update_time == updatetime)
		return;

	this->last_update_time = updatetime;

	sz_log(DATABLOCK_REFRESH_LOG_LEVEL, "CombinedDatablock::Refresh() '%ls'", this->GetBlockRelativePath().c_str());

	// load only new data
	int position = this->fixed_probes_count * sizeof(SZB_FILE_TYPE);
	TParam ** p_cache = this->param->GetFormulaCache();

	// msw file
	std::wstring tmppath = szb_datablock_t::GetBlockFullPath(buffer, p_cache[0], year, month);
	int size;
	try {
		size = fs::file_size(tmppath);
	} catch (fs::filesystem_error) {
		buffer->last_err = SZBE_SYSERR;
		return;
	}
	if (size <= position)	// check if file size changed
		return;

	int nsize = size; // new file size;

	tmppath = szb_datablock_t::GetBlockFullPath(buffer, p_cache[1], year, month);
	try {
		size = fs::file_size(tmppath);
	} catch (fs::filesystem_error) {
		buffer->last_err = SZBE_SYSERR;
		return;
	}
	if (size <= position)	// check if file size changed
		return;
	// choose shorter file
	if (size < nsize)
		nsize = size;

	// bytes to read
	nsize = (nsize - position);

	SZB_FILE_TYPE * buf_lsw = new SZB_FILE_TYPE[nsize];
	if (NULL == buf_lsw) {
		buffer->last_err = SZBE_OUTMEM;
		return;
	}

	int lsw_readed = szb_load_data(buffer, tmppath.c_str(),
				    (SZB_FILE_TYPE *) buf_lsw,
				    position, nsize);
	if (-1 == lsw_readed) {
		delete [] buf_lsw;
		return;
	}

	// msw file
	SZB_FILE_TYPE * buf_msw = new SZB_FILE_TYPE[nsize];
	if (NULL == buf_msw) {
		buffer->last_err = SZBE_OUTMEM;
		return;
	}

	tmppath = szb_datablock_t::GetBlockFullPath(buffer, p_cache[0], year, month);

	// we load only new data
	int msw_readed = szb_load_data(buffer, tmppath,
				    (SZB_FILE_TYPE *) buf_msw,
				    position, nsize);

	if (-1 == msw_readed) {
		delete [] buf_msw;
		delete [] buf_lsw;
		return;
	}

	if (msw_readed > lsw_readed) {
		// TODO - blad?
		msw_readed = lsw_readed;
	}

	// konwersja do SZB_TYPE
	double pw = pow(10, this->param->GetPrec());

	int old_c = this->fixed_probes_count;
	this->fixed_probes_count = old_c + msw_readed / sizeof(SZB_FILE_TYPE);

	for (int i = old_c; i < this->fixed_probes_count; i++) {
		if (SZB_FILE_NODATA == buf_msw[i - old_c])
			this->data[i] = SZB_NODATA;
		else {
			this->data[i] = (SZBASE_TYPE) ( (double)( 
				    int(szbfile_endian(buf_msw[i - old_c]) << 16)
				    	| (unsigned short)(szbfile_endian(buf_lsw[i - old_c]))
				    ) / pw );

			if (!IS_SZB_NODATA(this->data[i])) {
			    if (this->first_data_probe_index < 0)
				this->first_data_probe_index = i;

			    this->last_data_probe_index = i;
			}
		}
	}

	delete [] buf_msw;
	delete [] buf_lsw;

	return;
}

/** Loads data from szbase file.
 * @param buffer pointer to cache buffer
 * @param path path to file
 * @param pdata_buffer pointer to variable for return buffer
 * @param start start point of load
 * @param count how many bytes should be read
 * @return count of read bytes, -1 on error
 */
int
szb_load_data(szb_buffer_t * buffer, const std::wstring& path, SZB_FILE_TYPE * pdata_buffer,
	int start, int count)
{
#ifdef KDEBUG
    sz_log(9, "szb_load_data: %ls, s: %d, c: %d", path.c_str(), start, count);
#endif

#ifdef MINGW32
    /* Stupid, isnt't it? */
    int fd = open(SC::S2A(path).c_str(), O_RDONLY | O_BINARY);
#else
    int fd = open(SC::S2A(path).c_str(), O_RDONLY);
#endif

    if (fd == -1)
	return -1;
    
    /* for szb_combined_reload_block */
    /* move file pointer if start > 0 */
    if (start > 0) {
	if (lseek(fd, start, SEEK_SET) != start) {
	    sz_log(1, "szb_reload_block: lseek() failed");
	    close(fd);
	    return -1;
	}
    }

    int i = 0,
	to_read = count;

    while (to_read > 0) {
	i = read(fd, (char *) pdata_buffer + (count - to_read), to_read);
	if ((i >= 0) && (i < to_read)) {
	    lseek(fd, i, SEEK_SET);
	}
	if (i == 0)
	    break;
	if ((i < 0) && (errno != EINTR)) {
	    buffer->last_err = SZBE_SYSERR;
	    close(fd);
	    return -1;
	}
	if (i > 0)
	    to_read -= i;
    }
    close(fd);

    return count - to_read;
}
