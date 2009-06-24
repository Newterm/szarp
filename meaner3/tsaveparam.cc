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
{
	this->cname = wchar2szb(p->GetName());
}

TSaveParam::TSaveParam(const std::wstring& name)
{
	this->cname = wchar2szb(name);
}

TSaveParam::~TSaveParam()
{
}
		
int TSaveParam::Write(const fs::wpath& directory, time_t t, short int data, TStatus *status, 
		int overwrite, int force_nodata)
{
	int fd;			/* file descriptor */
	std::wstring filename;  /* name of file */
	std::wstring path;      /* full path to file */
	int month, year;	/* month and year of save */
	off_t index;		/* index of save in file */
	off_t last = -1;	/* last index in file */
	int ret;		/* temporary value */
	short int tmp;		/* buffer for write() */

	/* get index */
	index = szb_probeind(t);
	assert(index >= 0);
	sz_log(10, "meaner3: index in file: %ld", index);

	ret = szb_time2my(t, &year, &month);
	assert(ret == 0);
	
	/* do not write empty values */ 
	if (data == SZB_FILE_NODATA) {
		if (force_nodata == 0) {
			return 0;
		}
	} else if (status)
	/* increase counter of non-null values */
		status->Incr(TStatus::PT_NNPS);
	sz_log(10, "meaner3: writing '%d' to '%ls'", data, cname.c_str());
	
	/* get file name */
	filename = szb_createfilename_ne(cname, year, month);
	path = (directory / filename).string();
		
	/* check for file size */
	try {
		last = fs::file_size(path) / sizeof(short int);
		if (last > index && overwrite == 0) {
			sz_log(1, "meaner3: cannot overwrite data in file '%ls'",
					path.c_str());
			return 1;
		}

	} catch (fs::wfilesystem_error) 
	{ }

	/* make sure directory exists */
	if (szb_cc_parent(path)) {
		sz_log(1, "meaner3: error creating directory for file '%ls', errno %d",
				path.c_str(), errno);
		return 1;
	}
	/* open file */
	fd = open(SC::S2A(path).c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		sz_log(1, "meaner3: error opening file '%ls', errno %d",
				path.c_str(), errno);
		return 1;
	}

	/* check for file size */
	if (last < 0) 
		last = lseek(fd, 0, SEEK_END) / sizeof (short int);
	assert( last >= 0 );

	/* write NO_DATA in empty places */
	if (last < index) {
		ret = lseek(fd, last * sizeof(short int), SEEK_SET);
		assert (ret == (int)(last * sizeof(short int)) );
		tmp = SZB_FILE_NODATA;
		for ( ; last < index; last++)
			if (write(fd, &tmp, sizeof(tmp)) < 0) {
				sz_log(1, "meaner3: error writing (1) to file '%ls', errno %d",
						path.c_str(), errno);
				close(fd);
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
	if (write(fd, &data, sizeof(data)) < 0) {
		sz_log(1, "meaner3: error writing (2) to file '%ls', errno %d",
				path.c_str(), errno);
		close(fd);
		return 1;
	} 

	/* close file */
	close(fd);
		
	return 0;
}

