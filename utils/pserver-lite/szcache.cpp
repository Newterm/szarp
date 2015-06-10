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
#include <memory>

/** C-style */
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

/** Constants */

const int SzCache::cSzCacheProbe = 10;
const int SzCache::cSzCacheSize = 2;
const int16_t SzCache::cSzCacheNoData = -32768;
const std::string SzCache::cSzCacheExt = ".szc";

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
		/** Constructor only stores path */
		SzCacheFile(SzPath path) : path_(path) {}
		
		SzIndexResult cacheSearchRight(SzIndex sind, SzIndex eind) 
		{
			for (; sind < eind; sind++) {
				int16_t val = records[sind];
				if (val != cSzCacheNoData)
					return SzIndexResult(true,sind);
			}
			return SzIndexResult(false,-1);
		}

		SzIndexResult cacheSearchLeft(SzIndex sind, SzIndex eind) 
		{	
			for (; sind > eind; sind--) {
				int16_t val = records[sind];
				if (val != cSzCacheNoData)
					return SzIndexResult(true,sind);
			}
			return SzIndexResult(false,-1);
		}

		/** Open szbfile and store in records of values */
		void cacheOpen() 
		{
			std::ifstream fs(path_, std::ios::binary);
			if (fs.rdstate() & std::ifstream::failbit) 
				throw std::runtime_error("SzCacheFile: failed to open: " + path_);
			/* 
			 * File stream read method expects "char_type" pointer.
			 * A chosen (uint16_t) type can be reinterpreted to an
			 * array of char of sizeof type. It is filled with bytes
                         * and can be later used. (hary)
			 */
			records.clear(); 
			int16_t record;
			while (fs.read(reinterpret_cast<char*>(&record), sizeof record)) {
				records.push_back(record);	
			}
		};
		
		/** Map to memory szbfile and store in records of values */
		void cacheMap(off_t offset, size_t length) 
		{
			records.clear();

			off_t pg_off = (offset * sizeof(int16_t)) & ~(sysconf(_SC_PAGE_SIZE) - 1);
			size_t pg_len = (length + offset) * sizeof(int16_t) - pg_off;
        		int fd = open(path_.c_str(), O_RDONLY | O_NOATIME);
       			if (fd == -1) 
				throw std::runtime_error("SzCacheFile: failed to open: " + path_);

        		void* mdata = mmap(NULL, pg_len, PROT_READ, MAP_SHARED, fd, pg_off);

			short* data = (short*)(((char *)mdata) + (offset * sizeof(int16_t) - pg_off));
        		if (MAP_FAILED == data) {
                		close(fd);
				throw std::runtime_error("SzCacheFile: failed to mmap: " + path_);
        		}
			
			records.assign(data, data + length);
			close(fd);
			munmap(mdata,length);
		};

		/** 
		 * Map to memory szbfile and store in records of values 
		 * (hary 10.06.2015: This is used everywere now - map whole file,
		 * change this if too much RAM is used.
		 */
		void cacheMap() 
		{
			cacheMap(0, getFileSize(path_) / sizeof(int16_t));
		};

		/** Get value from record by index */
		int16_t &operator[](int i) 
		{
          		if (i > records.size()) 
				throw std::out_of_range("SzCacheFile: out_of_range");	
          		return records[i];
          	};
		
		
	private:
		/** Not using mmap() allocator (hary 7.06.2015) */
		//std::vector<int, mmap_allocator<int> > records;
		std::vector<int16_t> records;
		std::string path_;
};

/** Move month in path forward else backward */
SzCache::SzPath SzCache::moveMonth(SzPath path, bool forward) 
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
SzCache::SzPath SzCache::nextMonth(SzPath path) 
{
	return moveMonth(path, true);
}

/** Iterate over prev path also return it */
SzCache::SzPath SzCache::prevMonth(SzPath path) 
{
	return moveMonth(path, false);
}

SzCache::SzRange SzCache::availableRange() const
{
	SzCache::SzTime last = std::time(nullptr);
	auto gmt = std::gmtime(&last);
	gmt->tm_mon -= _numMonths;
	if (gmt->tm_mon < 1) {
		gmt->tm_mon += 12;
		gmt->tm_year -= 1;
	}
	gmt->tm_mday = 1;
	gmt->tm_hour = 0;
	gmt->tm_min = 0;
	gmt->tm_sec = 0;

	return SzRange(std::mktime(gmt), last);
}

SzCache::SzPathIndex SzCache::getPathIndex(const SzTime szt, const SzPath dir) const
{
	auto gmt = std::gmtime(&szt);
	std::ostringstream os;
	os << dir << "/" << std::setfill('0') << std::setw(4) << gmt->tm_year+1900 
		<< std::setw(2) << gmt->tm_mon+1 << cSzCacheExt;
	
	int idx = (gmt->tm_mday - 1) * 24 * 3600 + gmt->tm_hour * 3600 + gmt->tm_min * 60 + gmt->tm_sec;
	idx = std::floor(idx / cSzCacheProbe);

	return SzPathIndex(SzPath(os.str()),SzIndex(idx));
}

SzCache::SzTime	SzCache::getTime(SzIndex idx, SzPath path) const
{
	int year, month;
	std::tm gmt;

	if (idx == -1) 
		idx = getFileSize(path) / cSzCacheSize - 1;

	std::pair<std::string,std::string> p = splitPath(path);
	/* @TODO: some error checking on this stream */
	std::istringstream(p.second.substr(0,4)) >> gmt.tm_year;
	std::istringstream(p.second.substr(4,6)) >> gmt.tm_mon;
	gmt.tm_year -= 1900; // Falling back to years since 1900 repr.
	gmt.tm_mon -= 1; // Falling back to mon 0-11 repr.
	gmt.tm_mday = 1;
	gmt.tm_hour = 1;
	gmt.tm_min = 0;
	gmt.tm_sec = 0;
	gmt.tm_isdst = -1;

	return SzTime(std::mktime(&gmt) + (idx * cSzCacheProbe));
}

bool SzCache::validatePathMember(const std::string member) const
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

SzCache::SzPath SzCache::checkPath(const SzPath path) const
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

std::pair<std::string,std::string> SzCache::splitPath(const SzPath& path) const
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
	for (int i=0; i < res.gl_pathc; ++i)
		s.insert(std::string(res.gl_pathv[i]));
		
	globfree(&res);
	return s;
}

SzCache::SzRange SzCache::searchFirstLast(const SzPath path) const
{
	std::set<std::string> s = globify(path + std::string("/[0-9][0-9][0-9][0-9][0-9][0-9]") + cSzCacheExt);
	if (s.size() == 0) return SzRange(SzTime(-1),SzTime(-1));
	
	std::set<std::string>::const_iterator beg = s.begin();
	std::set<std::string>::iterator end = beg;
	std::advance(end, s.size() - 1);
	//std::cout <<"searchFirstLast("<<getTime(0,*beg)<<","<<getTime(-1,*beg)<<")\n";
	return SzRange (getTime(0,*beg),getTime(-1,*end));
}

SzCache::SzTime SzCache::searchAt(const SzTime start, const SzPath path) const 
{
	SzPathIndex szpi = getPathIndex(start, path);
	if (!fileExists(szpi.first)) 
		return SzTime(-1);

	SzCacheFile szf(szpi.first);
	try { 
		//szf.cacheOpen();
		szf.cacheMap();
		if (szf[szpi.second] == cSzCacheNoData) {
			return SzTime(-1);
		}	

	} catch (std::exception& e) {
		return SzTime(-1);
	}
	return start;
}

SzCache::SzIndex SzCache::lastIndex(const SzPath path) const
{
	return ( getFileSize(path) / cSzCacheSize - 1 );
}

SzCache::SzTime SzCache::searchFor(SzTime start, SzTime end, SzDirection dir, SzPath path)
{
	std::cout<<"searchFor("<<start<<","<<end<<","<<static_cast<int>(dir)<<","<<path<<")\n";
	SzPathIndex spi = getPathIndex(start, path);
	SzPathIndex epi = getPathIndex(end, path);

	int dirmod = (dir == SzDirection::RIGHT) ? 1 : -1;

	while ((start * dirmod) <= (end * dirmod)) {
		std::cout<<"loop("<<start*dirmod<<","<<end*dirmod<<")\n";
		SzIndexResult szir = searchFile(spi.first, spi.second, epi.first, epi.second, dir);
		start = szir.second;
		if (szir.first) return start;

		if (dirmod > 0) {
			spi.first = nextMonth(spi.first);
			start = getTime(0, spi.first);
			spi.second = 0;
		} else {
			start = getTime(0, spi.first) - cSzCacheProbe;
			spi.first = prevMonth(spi.first);
			spi.second = -1;
		}
	}
	return -1;
}

SzCache::SzIndexResult SzCache::searchFile(SzPath spath, SzIndex sind, SzPath epath, SzIndex eind, SzDirection dir)
{
	std::cout<<"searchFile("<<spath<<","<<sind<<","<<epath<<","<<eind<<","<<static_cast<int>(dir)<<")\n";

	if (!fileExists(spath)) 
		return SzIndexResult(false, SzIndex(-1));

	if (epath.compare(spath) != 0) {
		if (dir == SzDirection::RIGHT || dir == SzDirection::INPLACE)
			eind = lastIndex(spath);
		else {
			sind = std::min(eind, lastIndex(spath));
			eind = 0;
		}
	} else {
		if (dir == SzDirection::RIGHT) 
			eind = std::min(eind, lastIndex(spath));
		else
			sind = std::min(sind, lastIndex(spath));
	}

	if ((sind == SzIndex(-1)) && (dir == SzDirection::LEFT))
		sind = lastIndex(spath);
	
	SzCacheFile scf(spath);
	scf.cacheMap();	
	if (dir == SzDirection::RIGHT)
		return scf.cacheSearchRight(sind, eind);
	else
		return scf.cacheSearchLeft(sind, eind);

}

SzCache::SzSearchResult SzCache::search(SzTime start, SzTime end, SzDirection dir, SzPath path) 
{
	std::cout <<"search("<<start<<","<<end<<","<<static_cast<int>(dir)<<","<<path<<")\n";

	SzPath goodPath = checkPath(path);
	if (!directoryExists(goodPath)) 
		return SzSearchResult(-1,-1,-1);

	SzRange szr = searchFirstLast(goodPath);
	if (dir == SzDirection::INPLACE) {
		if (start == -1) start = szr.first;
		return SzSearchResult(searchAt(start, goodPath), szr.first, szr.second);
	}

	if (dir == SzDirection::RIGHT) {
		if ((start == -1) || (start < szr.first)) 
			start = szr.first;
		if ((end == -1) || (end > szr.second))
			end = szr.second;
	} else {
		if ((start == -1) || (start > szr.second))
			start = szr.second;
		if ((end == -1) || (end < szr.first)) 
			end = szr.first;
	}
	
	return SzSearchResult(searchFor(start, end, dir, goodPath), szr.first, szr.second);	
}

SzCache::SzSizeAndLast SzCache::getSizeAndLast(SzTime start, SzTime end, SzPath path) 
{
	SzPath goodPath = checkPath(path);
	if (!directoryExists(goodPath)) 
		return SzSizeAndLast(0,-1);

	SzRange szr = searchFirstLast(goodPath);
	if (szr.first == -1)
		return SzSizeAndLast(0,-1);

	return SzSizeAndLast((((end - start) / cSzCacheProbe) + 1) * cSzCacheSize, szr.second);	
}

void SzCache::writeFile(std::vector<int16_t>& vals, SzIndex sind, SzIndex eind, SzPath path)
{
	SzIndex lind = lastIndex(path);
	lind = std::min(lind, eind);
	std::size_t wcount = lind - sind + 1;
	
	if (wcount < 0) wcount = 0;
	
	if (wcount > 0) {
		SzCacheFile szf(path);
		szf.cacheMap();
		while (wcount > 0) {
			vals.push_back(szf[sind++]);
			--wcount;
		}
	}	
}

void SzCache::fillEmpty(std::vector<int16_t>& vals, std::size_t count) 
{
	for(int i = 0; i < count; ++i) vals.push_back(cSzCacheNoData);
}

SzCache::SzTime SzCache::writeData(std::vector<int16_t>& vals, SzTime start, SzTime end, SzPath path)
{
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
