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
 * szbase - szbfile.h
 * $Id$
 * <pawel@praterm.com.pl>
 */

#ifndef __SZBFILE_H__
#define __SZBFILE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

#include <sys/types.h>

#include <boost/filesystem/path.hpp>

#include "szbdefines.h"
#include <unistd.h>

/**
 * Creates SzarpBase file name from name of param, year and month. 
 * @param param_name name of param
 * @param year year, between 0 and 9999
 * @param month month, between 1 and 12
 * @return SzarpBase file name, allocated with malloc
 */
std::wstring
szb_createfilename(const std::wstring& param_name, const int year, const int month);

/**
 * Same as szb_createfilename, but param name is not recoded. Usefull
 * if you have already recoded param name.
 * @param suffix suffix of file name
 * @see szb_createfilename
 */
std::wstring
szb_createfilename_ne(const std::wstring& cparam, const int year, const int month, const wchar_t* suffix = L".szb");

/** Check for existence and create all parent directories for path.
 * @param path path to file, all components but the last one are treated as
 * directories to create if they do not already exist
 * @return 0 when direcory exist or was successfully created, 1 when error
 * occured while creating directory (see errno for errors from mkdir),
 * 2 when path exists but is not a directory
 */
int
szb_cc_parent(const std::wstring &path);

/** Uses open() function to open or create file for given parameter and date.
 * @param directory base directory for file
 * @param param encoded name of parameter
 * @param year year (between 0 and 9999)
 * @param month month (between 1 and 12)
 * @param flags flags, passed to open() function
 * @return file descriptor (greater or equal 0), -1 on system error (see errno
 * for detailed error description), SZBE_INVDATE when date (year or month) is
 * invalid, SZBE_INVPARAM if creating file name from param name is not
 * possible.
 */
int
szb_open(const std::wstring& directory, const std::wstring& param, 
	const int year, const int month, 
	const int flags);

/** Same as szb_open, but uses already decoded param name.
 * @see szb_open */
int
szb_open_ne(const std::wstring& directory, const std::wstring& param,
	const int year, const int month, 
	const int flags);

/** Closes file with given descriptor. Currently only calls close().
 * @param fd file descriptor
 * @return 0 on success, SZBE_SYSERR on error (see errno for details)
 */
int
szb_close(const int fd);

/** Retrives year and month from file name. */
void
szb_path2date(const std::wstring& path, int* year, int* month);

bool
is_szb_file_name(const boost::filesystem::wpath& path);

bool
szb_file_exists(const std::wstring &filename, time_t *file_modification_date = NULL);

#endif
