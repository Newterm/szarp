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
#pragma interface
#ifndef _SSDIRWALK_H_
#define _SSDIRWALK_H_

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#ifndef MINGW32
#include <libgen.h>
#endif
#include <cstring>
#include <vector>

#include <ssstring.h>

#ifdef MINGW32
#include "mingw32_missing.h"

/**Implementation of dirname for MSW*/
char* dirname(const char *);
/**Implementation of symlink for MSW*/
int symlink(const char *oldpath, const char *newpath);
/**Implementation of symlink removal*/
int remove_symlnk(const char *path);
#endif

#include "tokens.h"
#include "ssexception.h"


using std::vector;

/**Exception thrown upon error on operating */
class FSException : public Exception {
public:
	FSException(ssstring what);
};

/**Represents a file system*/
class TPath {
	/**Path itself*/
	char* m_path;
	/**Type of object pointed by this path*/
	mutable uint16_t m_type;
	/**Modification time of object pointed
	 * be a path*/
	mutable time_t m_mtime;
	/**Size of object pointed by a path*/
	mutable uint32_t m_size;

	mutable bool m_stated;

	void Stat() const;

	public:

	struct less {
		bool operator()(const TPath &a, const TPath &b) {
			return std::strcmp(a.m_path, b.m_path) < 0;
		}
	};


	typedef uint16_t FILE_TYPE;
	/**Path points to a regular file*/
	static const FILE_TYPE TFILE;
	/**Path points to a directory*/
	static const FILE_TYPE TDIR;
	/**Path points to a link*/
	static const FILE_TYPE TLINK;
	/**Unknown/unsupported type*/
	static const FILE_TYPE TUNKNOWN;
	/**Path does not represent an exisitng file*/
	static const FILE_TYPE TNOTEXISTS;
	/**Path exists but we don't have sufficient permissions
	 * to stat it*/
	static const FILE_TYPE TNACCESS;

	/**Creates an empty object*/
	TPath();

	/**Creates a path from given string stats
	 * for file type, size and modification time*/
	TPath(const char *path);

	/**@returns a path object to this path parent directory*/
	TPath DirName() const;
#ifndef MINGW32
	/**@returns a path object to this basename directory*/
	TPath BaseName() const;
#endif

	/**@return file modification time*/
	time_t GetModTime() const;

	/**@return file modification time*/
	uint32_t GetSize() const;

	/**If this path points to existing element in file system
	 * this method does nothing, otherwise a dir named m_path is created
	 */
	void CreateDir();

	TPath(const char* path, FILE_TYPE type, time_t modtime, uint32_t size);

	virtual ~TPath();

	TPath(const TPath& f);

	TPath& operator=(const TPath& f);

	/**@return string representig a path*/
	const char* GetPath() const;

	/**@return string representig a path*/
	const uint16_t GetType() const;

	/**performs concateaction of two path
	 * @param p path to concactate with this object*/
	TPath Concat(const TPath& p) const;

	bool IsSzbaseFile() const;

#ifndef MINGW32
	/**Finds a relative path from directory where param b is located
	 * to path pointed by param a. If param a in not an absolute path
	 * it's copy is returned.
	 * EXAMPLE :)
	 * a = /opt/szarp/zamo/szbase/Kociol_3
	 * b = /opt/szarp/zamX/szbase/Kociol_3
	 * result = ../../zamo/szbase/Kociol_3
	 * @return relative path string, shall be freed by a caller*/
	static char* Relative(const char* a, const char *b);
#endif

};

/**Reads data from a file
 * @param f file to read from
 * @param buf buffer to write data to
 * @param size size of the buffer
 * @return number of read bytes, is less then value of size
 * only if EOF is encountered*/
size_t ReadFully(FILE* f, void *buf, size_t size);

/**Writes data to a file 
 * @param f file to write to
 * @param buf buffer with data to be written
 * @param size size of the buffer*/
void WriteFully(FILE* f, void* buf, size_t size);

#ifndef MINGW32
/**Interface for file matching class utilized by @see ScanDir function*/
class FileMatch {
public:
	/**Match operation
	 * @param path to matched
	 * @return shall return true if this path is to be in result of @see ScanDir*/
	virtual bool operator() (const TPath& f) const = 0;
	virtual ~FileMatch() {};
};

/**Scans directory, paths for which filter funtion object returns true are returned
 * in result
 * @param tdir  directory to scan
 * @param filter function object used for paths matching 
 * @return list of matched paths*/
vector<TPath> ScanDir(const TPath& tdir, const FileMatch& filter, bool recursive = true); 
#endif


#endif
