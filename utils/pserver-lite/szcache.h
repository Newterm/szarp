#ifndef SZCACHE
#define SZCACHE

#include <utility> 
#include <ctime>
#include <string>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <set>
#include <algorithm>

/* Linux style headers */
#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdexcept>
#include <fstream>

class SzCache {	

	public:
		static const int cSzCacheProbe;
		static const int cSzCacheSize;
		static const int cSzCacheNoData;

		static const char cSzCacheType;

		static const std::string cSzCacheExt;

		/** @TODO: From LPR */
		static const std::string cSzCacheDir;
		static const int cSzCacheMonthCnt;
			
		enum class SearchDirection			{ RIGHT=1, LEFT=-1, INPLACE=0 };
		typedef SearchDirection				SzDirection;

		typedef std::time_t				SzTime;
		typedef std::string				SzPath;
		typedef unsigned				SzIndex;

		typedef std::pair<SzTime,SzTime>		SzRange;
		typedef std::tuple<SzTime,SzTime,SzTime>	SzSearchResult;
		typedef std::pair<bool, SzIndex>		SzIndexResult;
		typedef std::pair<SzPath,SzIndex>		SzPathIndex;
		
		SzRange		availableRange();
		SzSearchResult	search(SzTime start, SzTime end, SzDirection dir, SzPath path);
			
		SzRange		searchFirstLast(SzPath path);
		SzPath		checkPath(SzPath path);
		SzTime		searchAt(SzTime start, SzPath path);
		SzTime		searchFor(SzTime start, SzTime end, SzDirection dir, SzPath path);
		SzIndexResult	searchFile(SzPath spath, SzIndex sind, SzPath epath, SzIndex eind, SzDirection dir);

		SzPathIndex	getPathIndex(SzTime szt, SzPath dir);
		SzTime		getTime(SzIndex idx, SzPath path);
		SzIndex		lastIndex(SzPath path);

		std::set<std::string>			globify(const SzPath& path);	

	private:	
		class SzCacheFile;
		
		static std::pair<std::string,std::string>	splitPath(const SzPath& path);
		static std::size_t				getFileSize(const SzPath& path);

		bool					validatePathMember(std::string member);
		bool					directoryExists(const SzPath& path);
		bool					fileExists(const SzPath& path);
};

#endif /*SZCACHE*/
