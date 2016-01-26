/**
 * Probes Server LITE is a part of SZARP SCADA software.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA. """
 *
 */

#ifndef SZCACHE
#define SZCACHE

#include <string>
#include <ctime>
#include <set>

/* Linux style headers */
#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <tuple>

#include "shm_connection.h"

class SzCache {	

	public:	
	
		/** Constansts */

		/** Number of seconds in probe */
		static const int cSzCacheProbe;
		/** Number of bytes in szcache value */
		static const int cSzCacheSize;
		/** Value for NO_DATA */
		static const int16_t cSzCacheNoData;
		/** SzCacheFile extension */
		static const std::string cSzCacheExt;

		/** Types */

		/** Time representation in epoch */
		typedef std::time_t				SzTime;
		/** Path to file/directory */
		typedef std::string				SzPath;
		/** Index of probe in cache file (from 0) */
		typedef int					SzIndex;

		/** Range of time in cache */
		typedef std::pair<SzTime,SzTime>		SzRange;

		/** Range of index in cache file */
		typedef std::pair<SzIndex,SzIndex>		SzIndexRange;

		/** Cache search result <0,1,2>:
		 *  0 - Timestamp of probe that was found,
		 *  1 - Timestamp of search range begin,
		 *  2 - Timestamp of search range end.
		 */
		typedef std::tuple<SzTime,SzTime,SzTime>	SzSearchResult;

		/** Cache index search result <0,1>:
		 *  0 - Search success/fail,
		 *  1 - Index found.
		 */
		typedef std::pair<bool, SzIndex>		SzIndexResult;

		/** Cache path-index search result <0,1>:
		 *  0 - Path to cache file containing index,
		 *  1 - Index in cache file.
		 */
		typedef std::pair<SzPath,SzIndex>		SzPathIndex;
	
		/** Cache size-and-last query result <0,1>:
		 *  0 - Size of queried file,
		 *  1 - Last index in queried file.
		 */
		typedef std::pair<std::size_t,SzTime>		SzSizeAndLast;
			
		/** Interface */

		SzCache(SzPath cacheRootDir=SzPath("/var/cache/szarp"), int numMonths=2):
			_cacheRootDir(cacheRootDir), _numMonths(numMonths) {};

		SzRange		availableRange() const;
	
		/** Search for single data probe */
		SzSearchResult	searchInPlace(SzTime start, SzPath path);
		/** Search for data since start to end (start > end) */
		SzSearchResult	searchLeft(SzTime start, SzTime end, SzPath path);
		/** Search for data since start to end (start < end) */
		SzSearchResult	searchRight(SzTime start, SzTime end, SzPath path);

		SzSizeAndLast   getSizeAndLast(SzTime start, SzTime end, SzPath path);
		SzTime		writeData(std::vector<int16_t>& vals, SzTime start, SzTime end, SzPath path);
		
	private:
		static int toReadFromShm(SzPath path);
		static int getShmPos();		
		static int getShmCount();		
		static std::vector<int16_t> getShmData(SzPath path);

		/** Search utils */
		SzRange		searchFirstLast(SzPath path) const;
		SzTime		searchAt(SzTime start, SzPath path) const;
		SzTime		searchFor(SzTime start, SzTime end, SzPath path) const;
			
		/** Filesystem utils */
		SzPath					checkPath(SzPath path) const;
		std::set<SzPath>			globify(const SzPath& path) const;
		bool					validatePathMember(std::string member) const;
		bool					directoryExists(const SzPath& path) const;
		bool					fileExists(const SzPath& path) const;
		static std::size_t			getFileSize(const SzPath& path);

		static std::pair<SzPath,SzPath> splitPath(const SzPath& path);

		/** Epoch time <-> CACHE file path  and index translations */
		static SzPathIndex getPathIndex(SzTime szt, SzPath dir);
		static SzTime getTime(SzIndex idx, SzPath path);
		static SzIndex lastIndex(SzPath path);

		SzPath		moveMonth(SzPath path, bool forward) const;
		SzPath		nextMonth(SzPath path) const;
		SzPath		prevMonth(SzPath path) const;

		/** Data read utils */
		void		writeFile(std::vector<int16_t>& vals, SzIndex sind, SzIndex eind, SzPath path);
		void		fillEmpty(std::vector<int16_t>& vals, std::size_t count);
		
		static void	logMsg(int level, std::string msg);
	
		/** CACHE file representation */
		class SzCacheFile;

		static ShmConnection _shm_conn;

		SzPath _cacheRootDir;
		int _numMonths;
};

#endif /*SZCACHE*/
