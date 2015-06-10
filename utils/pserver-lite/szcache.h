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

		/** @TODO: From LPR */

		/** SzCache root directory */
		static const std::string cSzCacheDir;
		/** Number of months cached */
		static const int cSzCacheMonthCnt;
			
		/** Types */

		/** Search direction in cache files */
		enum class SearchDirection			{ RIGHT=1, LEFT=-1, INPLACE=0 };
		typedef SearchDirection				SzDirection;
		
		/** Time representation in epoch */
		typedef std::time_t				SzTime;
		/** Path to file/directory */
		typedef std::string				SzPath;
		/** Index of probe in cache file (from 0) */
		typedef int					SzIndex;

		/** Range of time in cache */
		typedef std::pair<SzTime,SzTime>		SzRange;

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

		SzRange		availableRange();
		SzSearchResult	search(SzTime start, SzTime end, SzDirection dir, SzPath path);
		SzSizeAndLast   getSizeAndLast(SzTime start, SzTime end, SzPath path);
		SzTime		writeData(std::vector<int16_t>& vals, SzTime start, SzTime end, SzPath path);
			
	private:	
		class SzCacheFile;

		std::set<std::string>			globify(const SzPath& path);
		std::pair<std::string,std::string>	splitPath(const SzPath& path);
		static std::size_t			getFileSize(const SzPath& path);
	
		bool					validatePathMember(std::string member);
		bool					directoryExists(const SzPath& path);
		bool					fileExists(const SzPath& path);

		SzRange		searchFirstLast(SzPath path);
		SzPath		checkPath(SzPath path);
		SzTime		searchAt(SzTime start, SzPath path);
		SzTime		searchFor(SzTime start, SzTime end, SzDirection dir, SzPath path);
		SzIndexResult	searchFile(SzPath spath, SzIndex sind, SzPath epath, SzIndex eind, SzDirection dir);
		SzPathIndex	getPathIndex(SzTime szt, SzPath dir);
		SzTime		getTime(SzIndex idx, SzPath path);
		SzIndex		lastIndex(SzPath path);

		SzPath		moveMonth(SzPath path, bool forward);
		SzPath		nextMonth(SzPath path);
		SzPath		prevMonth(SzPath path);

		void		writeFile(std::vector<int16_t>& vals, SzIndex sind, SzIndex eind, SzPath path);
		void		fillEmpty(std::vector<int16_t>& vals, std::size_t count);
};

#endif /*SZCACHE*/
