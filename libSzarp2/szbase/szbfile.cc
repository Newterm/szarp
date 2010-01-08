/* 
  SZARP: SCADA software 

*/
/*
 * szbase
 * $Id$
 * <pawel@praterm.com.pl>
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

#ifdef MINGW32
#include "mingw32_missing.h"
#endif


#include <sstream>
#include <iomanip>

#ifdef MINGW32
#undef int16_t
typedef short int               int16_t;
#endif

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"
#include "conversion.h"

#include "szbfile.h"
#include "szbname.h"
#include "szbdate.h"

namespace fs = boost::filesystem;

std::wstring szb_createfilename_ne(const std::wstring &cparam, 
		const int year, 
		const int month, 
		const wchar_t* suffix)
{
	std::wstring ret;

	if (cparam.empty())
		return ret;

	if ((month < 1) || (month > 12))
		return ret;

	if ((year < SZBASE_MIN_YEAR) || (year > SZBASE_MAX_YEAR))
		return ret;

	std::wstringstream wss;
	wss << cparam <<
#ifndef MINGW32
		L"/"
#else
		L"\\"
#endif
		<< std::setw(4) << std::setfill(L'0') << year 
		<< std::setw(2) << std::setfill(L'0') << month
		<< suffix;

	ret = wss.str();

	return ret;
}

std::wstring szb_createfilename(const std::wstring& param_name, const int year, const int month)
{
	return szb_createfilename_ne(wchar2szb(param_name), year, month);
}

int szb_cc_parent(const std::wstring& path)
{
	fs::wpath tmppath(path);
	tmppath.remove_leaf();
	if(fs::exists(tmppath))
		return 0;
	return fs::create_directories(tmppath) ? 0 : 1;
}

int szb_open_ne(const std::wstring& directory, const std::wstring& param,
		const int year, const int month,
		const int flags)
{
	int fd;
	int mode = 0;

	if ((year < SZBASE_MIN_YEAR) || (year > SZBASE_MAX_YEAR) 
			|| (month < 1) || (month > 12))
		return SZBE_INVDATE;
	fs::wpath filename(szb_createfilename_ne(param, year, month));
	if (filename.empty())
		return SZBE_INVPARAM;

	fs::wpath path(directory);
	path /= filename;
	if (flags & O_CREAT) {
		szb_cc_parent(path.string());
		mode = SZBASE_CMASK; 
	}
#ifdef MINGW32
	fd = open(SC::S2A(path.string()).c_str(), flags | O_BINARY, mode);
#else
	fd = open(SC::S2A(path.string()).c_str(), flags, mode);
#endif
	/* on error fd is -1, so we can just return it */
	return fd;
}

int szb_open(const std::wstring& directory, const std::wstring& param, 
		const int year, const int month, 
		const int flags)
{
	return szb_open_ne(directory, wchar2szb(param), year, month, flags);
}

int szb_close(const int fd)
{
	return close(fd);
}

void szb_path2date(const std::wstring& path, int* year, int* month)
{
	fs::wpath tmppath(path);
	std::wstring datestring = fs::basename(tmppath);

	*year = *month = 0;

	int len = datestring.length();

	if (len != 6)
		return;
	int i = len - 1;

	while (i > 4) {
		if (iswdigit(datestring[i]) && iswdigit(datestring[i-1])) {
			*month = (datestring[i - 1] - L'0') * 10 +
				datestring[i] - L'0';
			i -= 2;
			break;
		}
		i--;
	}
	while (i > 2) {
		if (iswdigit(datestring[i]) && iswdigit(datestring[i - 1]) &&
				iswdigit(datestring[i - 2]) &&
				iswdigit(datestring[i - 3])) {
			*year = (datestring[i - 3] - L'0') * 1000 +
				(datestring[i - 2] - L'0') * 100 +
				(datestring[i - 1] - L'0') * 10 +
				datestring[i] - L'0';
			break;
		}
		i--;
	}
	if ((*year <= 0) || (*year > 9999) || (*month <= 0) || (*month > 12))
		*year = *month = 0;
}

bool
is_szb_file_name(const fs::wpath& path)
{
	const std::wstring& str = path.string();

	for (int i = 0; i < 6; i++)
		if (!iswdigit(str[i]))
			return false;
	if (str[6] != L'.')
		return false;
	if (str[7] != L's')
		return false;
	if (str[8] != L'z')
		return false;
	if (str[9] != L'b')
		return false;
	if (str[10] != 0)
		return false;
	return true;
}

bool
szb_file_exists(const std::wstring& filename, time_t *file_modification_date)
{
	try {
		time_t mt = fs::last_write_time(filename);
		if (file_modification_date)
			*file_modification_date = mt;
	} catch (fs::wfilesystem_error) {
		if (file_modification_date)
			*file_modification_date = -1;
		return false;
	}

	return true;
}
