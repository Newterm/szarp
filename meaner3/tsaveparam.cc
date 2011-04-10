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
 * meaner3 - daemon writing data to base in SzarpBase format
 * SZARP
 * tsaveparam.cc
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tsaveparam.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/stat.h>

#include "tstatus.h"
#include "conversion.h"
#include "liblog.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"

TSaveParam::TSaveParam(TParam* p) 
	: fd(-1)
{
	this->cname = wchar2szb(p->GetName());
}

TSaveParam::TSaveParam(const std::wstring& name , bool convert )
	: fd(-1) , cname(name)
{
	if( convert ) cname = wchar2szb(cname);
}

TSaveParam::~TSaveParam()
{
	CloseFile();
}
		
int TSaveParam::WriteProbes(const fs::wpath& directory, time_t t, short int* data, size_t data_count)
{
	return Write(directory, t, data, data_count, NULL, 1, 0, 10);
}

int TSaveParam::Write(const fs::wpath& directory, time_t t, short int data, TStatus *status,
		                int overwrite, int force_nodata, time_t probe_length)
{
	return Write(directory, t, &data, 1, status, overwrite, force_nodata, probe_length);
}

int TSaveParam::WriteBuffered(const fs::wpath& directory, time_t t, short int* data, size_t data_count, TStatus *status, 
		int overwrite, int force_nodata, time_t probe_length) {
	std::wstring filename;  /* name of file */
	std::wstring path;      /* full path to file */
	int month, year;	/* month and year of save */
	off_t index;		/* index of save in file */
	off_t last = -1;	/* last index in file */
	int ret;		/* temporary value */
	short int tmp;		/* buffer for write() */

	/* get index */
	index = szb_probeind(t, probe_length);
	assert(index >= 0);
	sz_log(10, "TSaveParam::Write(): index in file: %ld", index);

	ret = szb_time2my(t, &year, &month);
	assert(ret == 0);
	
	/* do not write empty values, just count it */ 
	if (!force_nodata || status) {
		bool data_found = false;
		for (size_t i = 0; i < data_count; i++) {
			if (data[i] != SZB_FILE_NODATA) {
				data_found = true;
				break;
			}
		}
		if (!data_found and !force_nodata) {
			return 0;
		} else if (data_found and status) {
			/* increase counter of non-null values */
			status->Incr(TStatus::PT_NNPS);
		}
	}
	
	/* get file name */
	filename = szb_createfilename_ne(cname, year, month, probe_length == SZBASE_DATA_SPAN ? L".szb" : L".szc");
	path = (directory / filename).string();
		

	sz_log(10, "TSaveParam::Write(): writing '%d' (and %d other elements) to '%ls'", 
			data[0], (int)data_count - 1, path.c_str());

	if (fd == -1 || last_path != path) {
		CloseFile();
		/* make sure directory exists */
		if (szb_cc_parent(path)) {
			sz_log(1, "TSaveParam::Write(): error creating directory for file '%ls', errno %d",
					path.c_str(), errno);
			return 1;
		}
		/* open file */
		fd = open(SC::S2A(path).c_str(), O_RDWR | O_CREAT | O_CLOEXEC, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
		if (fd == -1) {
			sz_log(1, "TSaveParam::Write(): error opening file '%ls', errno %d",
				path.c_str(), errno);
			return 1;
		}
		last_path = path;
	}

	/* check for file size */
	last = lseek(fd, 0, SEEK_END) / sizeof (short int);
	assert(last >= 0);
	if (last > index && overwrite == 0) {
		sz_log(1, "TSaveParam::Write(): cannot overwrite data in file '%ls'",
				path.c_str());
		return 1;
	}

	/* write NO_DATA in empty places */
	if (last < index) {
		ret = lseek(fd, last * sizeof(short int), SEEK_SET);
		assert (ret == (int)(last * sizeof(short int)) );
		tmp = SZB_FILE_NODATA;
		for ( ; last < index; last++)
			if (write(fd, &tmp, sizeof(tmp)) == -1) {
				sz_log(1, "TSaveParam::Write(): error writing (1) to file '%ls', errno %d",
						path.c_str(), errno);
				CloseFile();
				return 1;
			}
	} else { /* set file pointer */
		ret = lseek(fd, index * sizeof(short int), SEEK_SET);
		assert (ret == (int)(index * sizeof(short int)));
	}
		
	/* write data */
#ifdef SZARP_BIG_ENDIAN
	data = szbfile_endian(data);
#endif /* SZARP_BIG_ENDIAN */
	//TODO if we plan to use bigger data_count, we should put loop here to save all data
	if (write(fd, data, data_count * sizeof(*data)) != (int)(data_count * sizeof(*data))) {
		sz_log(1, "TSaveParam::Write(): error writing (2) to file '%ls', errno %d",
				path.c_str(), errno);
		CloseFile();
		return 1;
	} 
	return 0;

}

int TSaveParam::Write(const fs::wpath& directory, time_t t, short int* data, size_t data_count, TStatus *status, 
		int overwrite, int force_nodata, time_t probe_length)
{
	int ret = WriteBuffered(directory, t, data, data_count, status, overwrite, force_nodata, probe_length);
	if (ret)
		CloseFile();
	return ret;
}

void TSaveParam::CloseFile() {
	if (fd >= 0) {
		close(fd);
		fd = -1;
		last_path.clear();
	}
}

