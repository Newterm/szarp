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

#include "szcache.h"

#include <iostream>
#include <iomanip>
#include <utility> 
#include <sstream>
#include <cmath>
#include <cassert>
#include <memory>

/** C-style */
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/** Constants */

const int SzCache::cSzCacheProbe = 10;
const int SzCache::cSzCacheSize = 2;
const int16_t SzCache::cSzCacheNoData = -32768;
const std::string SzCache::cSzCacheExt = ".szc";

/** Static */
ShmConnection SzCache::_shm_conn;

/** mmap() allocator for std::vector */

template <class T> class mmap_allocator
{
	public: 
		typedef T			value_type;
		typedef value_type*		pointer;
		typedef const value_type*	const_pointer;
		typedef value_type&		reference;
		typedef const value_type&	const_reference;
		typedef std::size_t		size_type;
		typedef std::ptrdiff_t		difference_type;

		mmap_allocator() {}
		mmap_allocator(const mmap_allocator&) {}
		~mmap_allocator() {}

		pointer address (reference x) const 
		{ return &x; }
		const_pointer address (const_reference x) const
		{ return &x; } 

		pointer allocate (size_type n, void * hint = 0)
		{	
			void *p = mmap(hint, n*sizeof(T), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
			if (p == MAP_FAILED) throw std::bad_alloc();
			return static_cast<pointer>(p);
		}

		void deallocate(pointer p, size_type n) 
		{  munmap(p, n*sizeof(T)); }

		size_type max_size() const
		{ return static_cast<size_type>(-1) / sizeof(value_type); }

		void construct (pointer p, const value_type& x) 
		{ new(p) T(x); }

		void destroy (pointer p)
		{ p->~value_type(); }

		template <class U> 
		mmap_allocator (const mmap_allocator<U>&) {}
		template <class U>
		struct rebind { typedef mmap_allocator<U> other; };

	private:
		void operator= (const mmap_allocator&);
};

template <class T>
inline bool operator== (const mmap_allocator<T>&,
			const mmap_allocator<T>&)
{ return true; }

template <class T>
inline bool operator!= (const mmap_allocator<T>&,
			const mmap_allocator<T>&)
{ return false; }

/*
template<> class mmap_allocator<void>
{	
	typedef void		value_type;
	typedef void*		pointer;
	typedef const void*	const_pointer;

	template <class U> 
	struct rebind { typedef mmap_allocator<U> other; };
}
*/

/** SzSzache inner szc file representation */
class SzCache::SzCacheFile {
	public:
		void appendFromShm(SzPath path) 
		{
			int to_read = toReadFromShm(path);
			if (to_read > 0) {
				logMsg(9, "SzCache::cacheMap: to read from SHM: " + std::to_string(to_read));
				std::vector<int16_t> shm_data = getShmData(path);
					
				/*
				logMsg(9, "SzCache::cacheMap: values in SHM: " + std::to_string(shm_data.size()));
				std::ostringstream os;
				for(auto v = shm_data.begin(); v != shm_data.end(); ++v) os << " " << *v;
				os << " pos: " << getShmPos() << " count: " << getShmCount();
				logMsg(9, "SHM: [" + os.str() + "]");
				*/

				if ((size_t)to_read >= shm_data.size())
					_records.insert(_records.end(), shm_data.begin(), shm_data.end());
				else 
					_records.insert(_records.end(), shm_data.end() - to_read, shm_data.end());
				
				logMsg(9, "SzCache::cacheMap: file: " + path 
					+ " loaded probes + SHM: " + std::to_string(_records.size()));
			}
		}		
		SzIndexResult cacheSearchRight(SzIndex sind, SzIndex eind) 
		{
			logMsg(3, std::string("cacheSearchRight(") 
				+ std::to_string(sind) + std::string(",")
				+ std::to_string(eind) + std::string(")"));

			for (; sind <= eind; sind++) {
				if (sind < 0 || static_cast<size_t>(sind) >= _records.size()) 
					return SzIndexResult(false,-1);

				int16_t val = _records[sind];
				
				/*
				logMsg(3, std::string("[") 
					+ std::to_string(sind) + std::string("]:")
					+ std::to_string(val) + std::string(","));
				*/

				if (val != cSzCacheNoData)
					return SzIndexResult(true,sind);
			}
			return SzIndexResult(false,-1);
		}

		SzIndexResult cacheSearchLeft(SzIndex sind, SzIndex eind) 
		{
			logMsg(3, std::string("cacheSearchLeft(") 
				+ std::to_string(sind) + std::string(",")
				+ std::to_string(eind) + std::string(")"));

			for (; sind >= eind; sind--) {
				if (sind < 0 || static_cast<size_t>(sind) >= _records.size()) 
					return SzIndexResult(false,-1);

				int16_t val = _records[sind];
				
				/*
				logMsg(3, std::string("[") 
					+ std::to_string(sind) + std::string("]:")
					+ std::to_string(val) + std::string(","));
				*/

				if (val != cSzCacheNoData)
					return SzIndexResult(true,sind);
			}
			return SzIndexResult(false,-1);
		}

		SzIndexResult cacheSearch(SzIndex sind, SzIndex eind)
		{
			if (sind <= eind) return cacheSearchRight(sind,eind);
			else return cacheSearchLeft(sind,eind);
		}

		/** Open szbfile and store in _records of values */
		/** deprecated - use cacheMap() (hary 12.06.2015 */
		void cacheOpen(const SzPath& path) 
		{
			std::ifstream fs(path, std::ios::binary);
			if (fs.rdstate() & std::ifstream::failbit)
				throw std::runtime_error("SzCacheFile: failed to open: " + path);
			/* 
			 * File stream read method expects "char_type" pointer.
			 * A chosen (uint16_t) type can be reinterpreted to an
			 * array of char of sizeof type. It is filled with bytes
                         * and can be later used. (hary)
			 */
			_records.clear(); 
			int16_t record;
			while (fs.read(reinterpret_cast<char*>(&record), sizeof record)) {
				_records.push_back(record);	
			}
		};
		
		/** Map to memory szbfile and store in _records of values */
		void cacheMap(const SzPath& path, off_t offset, size_t length) 
		{
			_records.clear();

			off_t pg_off = (offset * sizeof(int16_t)) & ~(sysconf(_SC_PAGE_SIZE) - 1);
			size_t pg_len = (length + offset) * sizeof(int16_t) - pg_off;
        		int fd = open(path.c_str(), O_RDONLY | O_NOATIME);
       			if (fd == -1)
				throw std::runtime_error("SzCacheFile: failed to open: " + path);
        		void* mdata = mmap(NULL, pg_len, PROT_READ, MAP_SHARED, fd, pg_off);

			short* data = (short*)(((char *)mdata) + (offset * sizeof(int16_t) - pg_off));
        		if (MAP_FAILED == data) {
                		close(fd);
				throw std::runtime_error("SzCacheFile: failed to mmap: " + path);
        		}
			
			_records.assign(data, data + length);
			close(fd);
			munmap(mdata,length);
			
			logMsg(9, "SzCache::cacheMap: file: " + path 
				+ " loaded probes: " + std::to_string(_records.size()));
			
			appendFromShm(path);
			
		};

		/** 
		 * Map to memory szbfile and store in _records of values 
		 * (hary 10.06.2015: This is used everywere now - map whole file,
		 * change this if too much RAM is used.
		 */
		void cacheMap(const SzPath& path) 
		{
			logMsg(3, std::string("cacheMap(") + path
				+ std::string(")"));

			cacheMap(path, 0, getFileSize(path) / sizeof(int16_t));
		};

		/** Get value from record by index */
		int16_t &operator[](unsigned int i) 
		{
          		if (i > _records.size())
				throw std::out_of_range("SzCacheFile: out_of_range");
			/*
			logMsg(3, std::string("records[") + std::to_string(i) 
				+ std::string("]:") + std::to_string(_records[i]));
			*/

          		return _records[i];
          	};
		
		void setRecords(const std::vector<int16_t>& records) 
		{
			_records.clear();
			_records.assign(records.begin(),records.end());
		}

	private:
		/** Not using mmap() allocator (hary 7.06.2015) */
		//std::vector<int, mmap_allocator<int> > _records;
		std::vector<int16_t> _records;
};

/** Move month in path forward else backward */
SzCache::SzPath SzCache::moveMonth(SzPath path, bool forward) const
{
	int year, mon;
	std::ostringstream os;

	std::pair<std::string,std::string> p = splitPath(path);
	/* @TODO: some error checking on this stream */
	std::istringstream(p.second.substr(0,4)) >> year;
	std::istringstream(p.second.substr(4,6)) >> mon;

	forward ? mon += 1 : mon -= 1;
	if (mon > 12) { mon = 1; year += 1; }
	if (mon <= 0) { mon = 12; year -= 1; }

	os << p.first << "/" << std::setfill('0') << std::setw(4) << year 
		<< std::setw(2) << mon << cSzCacheExt;
	
	return os.str();
}

/** Iterate over next path also return it */
SzCache::SzPath SzCache::nextMonth(SzPath path) const
{
	logMsg(3, std::string("nextMonth(") + path + std::string(")"));
	return moveMonth(path, true);
}

/** Iterate over prev path also return it */
SzCache::SzPath SzCache::prevMonth(SzPath path) const
{
	logMsg(3, std::string("prevMonth(") + path + std::string(")"));
	return moveMonth(path, false);
}

SzCache::SzRange SzCache::availableRange() const
{
	/** Using UTC */

	/** Current epoch */
	SzCache::SzTime last = std::time(nullptr);

	/** Current epoch to UTC struct tm */
	std::tm *gmt = std::gmtime(&last);

	gmt->tm_mon -= _numMonths;
	if (gmt->tm_mon < 1) {
		gmt->tm_mon += 12;
		gmt->tm_year -= 1;
	}

	gmt->tm_mday = 1;
	gmt->tm_hour = 0;
	gmt->tm_min = 0;
	gmt->tm_sec = 0;
	
	/** UTC struct tm to epoch */
	return SzRange(timegm(gmt), last);
}

SzCache::SzPathIndex SzCache::getPathIndex( SzTime szt, SzPath dir)
{
	logMsg(3, std::string("getPathIndex(") + std::to_string(szt)
		+ std::string(",") + dir + std::string(")"));

	std::tm* gmt = std::gmtime(&szt);

	std::ostringstream os;

	os << dir << "/" << std::setfill('0') << std::setw(4) << gmt->tm_year+1900 
		<< std::setw(2) << gmt->tm_mon+1 << cSzCacheExt;
	
	int idx = (gmt->tm_mday - 1) * 24 * 3600 + gmt->tm_hour * 3600 + gmt->tm_min * 60 + gmt->tm_sec;

	idx = std::floor(idx / cSzCacheProbe);
	//idx = std::floor((idx - (idx % cSzCacheProbe)) / cSzCacheProbe);

	return SzPathIndex(SzPath(os.str()),SzIndex(idx));
}

SzCache::SzTime	SzCache::getTime(SzIndex idx, SzPath path)
{
	logMsg(3, std::string("getTime(") + std::to_string(idx)
		+ std::string(",") + path + std::string(")"));

	std::tm gmt;
	if (idx == -1) 
		idx = lastIndex(path);
		//idx = getFileSize(path) / cSzCacheSize - 1;

	std::pair<std::string,std::string> p = splitPath(path);
	/* @TODO: some error checking on this stream */
	std::istringstream(p.second.substr(0,4)) >> gmt.tm_year;
	std::istringstream(p.second.substr(4,6)) >> gmt.tm_mon;
	gmt.tm_year -= 1900; // Falling back to years since 1900 repr.
	gmt.tm_mon -= 1; // Falling back to mon 0-11 repr.
	gmt.tm_mday = 1;
	gmt.tm_hour = 0;
	gmt.tm_min = 0;
	gmt.tm_sec = 0;

	return SzTime(timegm(&gmt) + (idx * cSzCacheProbe));
}

bool SzCache::validatePathMember(std::string member) const
{
	if (	(member.compare(".") != 0) && 
		(member.compare("..") != 0) && 
		(member.compare("") != 0)) 
	{
		return true;

	} else {
		return false;
	}
}

SzCache::SzPath SzCache::checkPath(SzPath path) const
{
	bool goodPath = true;
	std::size_t memberCount = 0;

	std::size_t beg = 0;
	std::size_t end = path.find("/");
	while (end != std::string::npos) {
		if (goodPath) goodPath = validatePathMember(path.substr(beg,end-beg));
		memberCount++;
		beg = end+1;
		end = path.find("/",beg,1);
	}

	if (goodPath) { 
		goodPath = validatePathMember(path.substr(beg));
		memberCount++;
	}

	/* Directory path depth of 3 is hardcoded in SZARP */
	if (goodPath && (memberCount == 3)) 
		return SzPath(_cacheRootDir + std::string("/") + path);
	else
		return SzPath("");
}

std::size_t SzCache::getFileSize(const SzPath& path)
{
	struct stat res;
	if(stat(path.c_str(), &res) != 0) {
		return 0;
	}
	return res.st_size;
}

std::pair<std::string,std::string> SzCache::splitPath(const SzPath& path)
{
	std::size_t pos = path.rfind("/");
	if (pos != std::string::npos)
		return std::pair<std::string,std::string>(path.substr(0,pos),path.substr(pos+1));

	return std::pair<std::string,std::string>(std::string(""),std::string(""));
}

bool SzCache::directoryExists(const SzPath& path) const
{
	struct stat res;
	if(stat(path.c_str(), &res) != 0) {
		return false;
	}
	
	if (res.st_mode & S_IFDIR) 
		return true;
	
	return false;
}

bool SzCache::fileExists(const SzPath& path) const
{
	struct stat res;
	if(stat(path.c_str(), &res) != 0) {
		return false;
	}

	/* Accept symlinks as files */
	if ((res.st_mode & S_IFREG)||(res.st_mode & S_IFLNK)) 
		return true;
	
	return false;
}

std::set<std::string> SzCache::globify(const SzPath& path) const
{
	glob_t res;
	glob(path.c_str(), GLOB_TILDE, NULL, &res);
	std::set<std::string> s;
	for (unsigned int i=0; i < res.gl_pathc; ++i)
		s.insert(std::string(res.gl_pathv[i]));
		
	globfree(&res);
	return s;
}

SzCache::SzRange SzCache::searchFirstLast(SzPath path) const
{
	logMsg(3, std::string("searchFirstLast(") + path + std::string(")"));

	std::set<std::string> s = globify(path + std::string("/[0-9][0-9][0-9][0-9][0-9][0-9]") + cSzCacheExt);
	if (s.size() == 0) return SzRange(SzTime(-1),SzTime(-1));
	
	std::set<std::string>::const_iterator beg = s.begin();
	std::set<std::string>::iterator end = beg;
	std::advance(end, s.size() - 1);
	//std::cout <<"searchFirstLast("<<getTime(0,*beg)<<","<<getTime(-1,*beg)<<")\n";
	return SzRange (getTime(0,*beg),getTime(-1,*end));
}

SzCache::SzTime SzCache::searchAt(SzTime start, SzPath path) const 
{
	logMsg(3, std::string("searchAt(") + std::to_string(start)
		+ std::string(",") + path + std::string(")"));

	SzPathIndex szpi = getPathIndex(start, path);
	if (!fileExists(szpi.first)) 
		return SzTime(-1);

	SzCacheFile szf;
	try { 
		//szf.cacheOpen();
		szf.cacheMap(szpi.first);
		if (szf[szpi.second] == cSzCacheNoData) {
			return SzTime(-1);
		}	

	} catch (std::exception& e) {
		logMsg(1, "SzCache::cacheMap: Exception " + std::string(e.what()));
		return SzTime(-1);
	}
	return start;
}

SzCache::SzIndex SzCache::lastIndex(SzPath path)
{	
	int in_file = std::floor(getFileSize(path) / cSzCacheSize);
	int to_read = 0;
	try { 	
		to_read = toReadFromShm(path);
	} catch (std::runtime_error re) {
		logMsg(0, "SzCache::lastIndex: Exception: " + std::string(re.what()));	
	}
	return (in_file + to_read - 1);
}

SzCache::SzTime SzCache::searchFor(SzTime start, SzTime end, SzPath path) const
{
	logMsg(3, std::string("searchFor(") + std::to_string(start) 
		+ std::string(",") + std::to_string(end) 
		+ std::string(",") + path + std::string(")"));

	SzPathIndex startPathIndex = getPathIndex(start, path);
	SzPathIndex endPathIndex = getPathIndex(end, path);
	
	SzPath spath = startPathIndex.first;
	SzPath epath = endPathIndex.first;
	SzIndex sind = startPathIndex.second;
	SzIndex eind = endPathIndex.second;

	logMsg(3, spath + std::string(" idx:") + std::to_string(sind)
		+ std::string("-->>") 
		+ epath + std::string(" idx:") + std::to_string(eind));

	bool right = (start <= end);

	while ( right ? (spath.compare(nextMonth(epath))!=0) : 
			(spath.compare(prevMonth(epath))!=0))
	{
		if (fileExists(spath)) {
			SzCacheFile scf;

			try { scf.cacheMap(spath); } catch (std::runtime_error re) {
				// Some exception, but file exists so try to go on
				logMsg(1, "SzCache::cacheMap: Exception " + std::string(re.what()));
			}

			SzIndexResult szir; 
			
			if (spath.compare(epath)!=0)
				right ? szir = scf.cacheSearch(sind,lastIndex(spath)) :
					szir = scf.cacheSearch(std::min(sind,lastIndex(spath)),0);
			else
				right ?	szir = scf.cacheSearch(sind,std::min(eind,lastIndex(spath))) :
					szir = scf.cacheSearch(std::min(sind,lastIndex(spath)),eind);

			if (szir.first) return getTime(szir.second, spath);
		}
		sind =	right ? 0 : lastIndex(spath);
		spath = right ? nextMonth(spath) : prevMonth(spath);
	}
	return -1;
}

SzCache::SzSearchResult SzCache::searchInPlace(SzTime start, SzPath path) 
{
	logMsg(3, std::string("searchInPlace(") + std::to_string(start) 
		+ std::string(",") + path + std::string(")"));

	SzPath goodPath = checkPath(path);
	if (!directoryExists(goodPath)) 
		return SzSearchResult(-1,-1,-1);

	SzRange szr = searchFirstLast(goodPath);
	if (start == -1) start = szr.first;
		
	return SzSearchResult(searchAt(start, goodPath), szr.first, szr.second);
}

SzCache::SzSearchResult SzCache::searchLeft(SzTime start, SzTime end, SzPath path) 
{
	logMsg(3, std::string("searchLeft(") + std::to_string(start) 
		+ std::string(",") + std::to_string(end) + std::string(",")
		+ path + std::string(")"));

	SzPath goodPath = checkPath(path);
	if (!directoryExists(goodPath)) 
		return SzSearchResult(-1,-1,-1);

	SzRange szr = searchFirstLast(goodPath);

	if ((start == -1) || (start > szr.second))
		start = szr.second;
	if ((end == -1) || (end < szr.first)) 
		end = szr.first;
	
	if (end > szr.second) end = szr.second; 
	if (start < szr.first) start = szr.first;

	assert(start >= end);

	return SzSearchResult(searchFor(start, end, goodPath), szr.first, szr.second);	
}

SzCache::SzSearchResult SzCache::searchRight(SzTime start, SzTime end, SzPath path) 
{
	logMsg(3, std::string("searchRight(") + std::to_string(start) 
		+ std::string(",") + std::to_string(end) + std::string(",")
		+ path + std::string(")"));

	SzPath goodPath = checkPath(path);
	if (!directoryExists(goodPath)) 
		return SzSearchResult(-1,-1,-1);

	SzRange szr = searchFirstLast(goodPath);

	if ((start == -1) || (start < szr.first)) 
		start = szr.first;
	if ((end == -1) || (end > szr.second))
		end = szr.second;
	
	if (end < szr.first) end = szr.first; 
	if (start > szr.second) start = szr.second;

	assert(start <= end);

	return SzSearchResult(searchFor(start, end, goodPath), szr.first, szr.second);	
}

SzCache::SzSizeAndLast SzCache::getSizeAndLast(SzTime start, SzTime end, SzPath path) 
{
	SzPath goodPath = checkPath(path);
	if (!directoryExists(goodPath)) 
		return SzSizeAndLast(0,-1);

	SzRange szr = searchFirstLast(goodPath);
	if (szr.first == -1)
		return SzSizeAndLast(0,-1);

	return SzSizeAndLast(((std::abs(end - start) / cSzCacheProbe) + 1) * cSzCacheSize, szr.second);	
}

void SzCache::writeFile(std::vector<int16_t>& vals, SzIndex sind, SzIndex eind, SzPath path)
{
	logMsg(3, std::string("writeFile(") 
		+ std::to_string(sind) + std::string(",")
		+ std::to_string(eind) + std::string(",")
		+ path + std::string(")"));

	SzIndex lind = std::min(lastIndex(path), eind);
	int wcount = lind - sind + 1;
	
	logMsg(3, std::string("lind:") + std::to_string(lind)
		+ std::string(" wcount:") + std::to_string(wcount));

	if (wcount < 0) wcount = 0;
	if (wcount > 0) {
		SzCacheFile szf;
		szf.cacheMap(path);
		while (wcount > 0) {

			try { 
				vals.push_back(szf[sind]);
			} catch (std::exception& e) {
				logMsg(1, "SzCache::writeFile:" + std::string(e.what()));
				vals.push_back(cSzCacheNoData);
			}

			sind++;
			--wcount;
		}
		//fillEmpty(vals, eind - sind - wcount + 1);
	}
	fillEmpty(vals, eind - sind - wcount + 1);
}

void SzCache::fillEmpty(std::vector<int16_t>& vals, std::size_t count) 
{
	logMsg(3, std::string("fillEmpty(") + std::to_string(count) + std::string(")"));
	for(unsigned int i = 0; i < count; ++i) vals.push_back(cSzCacheNoData);
}

SzCache::SzTime SzCache::writeData(std::vector<int16_t>& vals, SzTime start, SzTime end, SzPath path)
{
	logMsg(3, std::string("writeData(") 
		+ std::to_string(start) + std::string(",")
		+ std::to_string(end) + std::string(",")
		+ path + std::string(")"));

	SzPath goodPath = checkPath(path);
	if (!directoryExists(goodPath))
		return (end + 1);
	
	SzPathIndex spi = getPathIndex(start, goodPath);
	SzPathIndex epi = getPathIndex(end, goodPath);
	if (spi.first.compare(epi.first) == 0) {
		writeFile(vals, spi.second, epi.second, spi.first);
		return (getTime(epi.second, spi.first) + cSzCacheProbe);
	} else {
		SzPath npath = nextMonth(spi.first);
		SzTime ntime = getTime(0, npath);

		epi = getPathIndex(ntime - cSzCacheProbe, goodPath);
		writeFile(vals, spi.second, epi.second, spi.first);
		return ntime;
	}
}

int SzCache::toReadFromShm(SzPath path)
{	
	SzTime time_now = std::time(nullptr);
		
	SzPath dir_path = path.substr(0, path.rfind("/"));
	SzPathIndex path_index_now = getPathIndex(time_now, dir_path);
	
	if (path_index_now.first.compare(path) != 0) {
		logMsg(9, "SzCache::toReadFromShm: File: " + path + " not current so no SHM needed");
		return 0;
	}

	// Hack - remove the additional .szc after getPathIndex call
	//szpi_now.first = szpi_now.first.substr(0, szpi_now.first.rfind("/"));
	//logMsg(5, "path: " + path + " now: " + szpi_now.first);

	int expected = path_index_now.second;
	logMsg(9, "SzCache::toReadFromShm: Expecting " + std::to_string(expected) + " probes in file");

	if (expected < 0) return 0;
		//throw std::runtime_error("SzCache::toReadFromShm: expected < 0");
	
	int in_file = std::floor(getFileSize(path) / sizeof(int16_t));
	logMsg(9, "SzCache::toReadFromShm: " + path + " = " + std::to_string(in_file) + " probes");
	
	int probes_diff = expected - in_file;
	
	if (probes_diff < 0) return 0;
		//throw std::runtime_error("SzCache::toReadFromShm: probes_diff < 0");
			
	logMsg(9, "SzCache::toReadFromShm: To be loaded frm SHM: " + std::to_string(probes_diff));
	
	return probes_diff;
}

int SzCache::getShmPos()
{
	return _shm_conn.get_pos();
}
 	
int SzCache::getShmCount()		
{
	return _shm_conn.get_count();
}

std::vector<int16_t> SzCache::getShmData(SzPath path)
{
	int index = -1;
		
	if (!_shm_conn.registered()) _shm_conn.register_signals();
	if (!_shm_conn.configured()) _shm_conn.configure();
	if (!_shm_conn.connected()) _shm_conn.connect();

	_shm_conn.update_segment();
	
	try {
		index = _shm_conn.param_index_from_path(path);
	} catch (std::runtime_error e) {
		logMsg(0, "SzCache::getShmData: Failed to get shared memory for %s" + path);
	}
	
	return _shm_conn.get_values(index);
}

void SzCache::logMsg(int level, std::string msg)
{
	std::cout << msg << std::endl;
}

