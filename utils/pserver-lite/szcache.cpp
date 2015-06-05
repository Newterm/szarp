#include "szcache.h"
#include <iostream>

const int SzCache::cSzCacheProbe = 10;
const int SzCache::cSzCacheSize = 2;
const int SzCache::cSzCacheNoData = -32768;

const char SzCache::cSzCacheType = 'h';

const std::string SzCache::cSzCacheExt = ".szc";
/** @TODO: From LPR */
const std::string SzCache::cSzCacheDir = "/var/cache/szarp";
const int SzCache::cSzCacheMonthCnt = 2;


/** SzSzache inner szc file representation */
class SzCache::SzCacheFile {

	public:
		/** Constructor only stores path */
		SzCacheFile(SzPath path) : path_(path) {};
		
		/** Open szbfile and store in records of values */
		void open() 
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
			std::uint16_t record;
			while (fs.read(reinterpret_cast<char*>(&record), sizeof record)) {
				records.push_back(record);	
			}
		};
		
		/** Get value from record by index */
		int &operator[](int i) 
		{
          		if (i > records.size()) 
				throw std::out_of_range("SzCacheFile: out_of_range");	
          		return records[i];
          	};
		
		/** Move month in path forward else backward */
		std::string moveMonth(bool forward) 
		{
			int year, mon;
			std::ostringstream os;

			std::pair<std::string,std::string> p = splitPath(path_);
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
		std::string next() 
		{
			path_ = moveMonth(true);
			return path_;
		}

		/** Iterate over prev path also return it */
		std::string prev() 
		{
			path_ = moveMonth(false);
			return path_;
		}

	private:
		std::vector<int> records;
		std::string path_;
};

SzCache::SzRange SzCache::availableRange() 
{
	SzCache::SzTime last = std::time(nullptr);
	auto gmt = std::gmtime(&last);
	gmt->tm_mon -= cSzCacheMonthCnt;
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

SzCache::SzPathIndex SzCache::getPathIndex(SzTime szt, SzPath dir)
{
	auto gmt = std::gmtime(&szt);
	std::cout << szt << std::endl;
	std::ostringstream os;
	os << dir << "/" << std::setfill('0') << std::setw(4) << gmt->tm_year+1900 
		<< std::setw(2) << gmt->tm_mon+1 << cSzCacheExt;
	
	int idx = (gmt->tm_mday - 1) * 24 * 3600 + gmt->tm_hour * 3600 + gmt->tm_min * 60 + gmt->tm_sec;
	idx = std::floor(idx / cSzCacheProbe);

	return SzPathIndex(SzPath(os.str()),SzIndex(idx));
}

SzCache::SzTime	SzCache::getTime(SzIndex idx, SzPath path)
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
	gmt.tm_hour = 0;
	gmt.tm_min = 0;
	gmt.tm_sec = 0;
	//gmt.tm_isdst = -1;

	return SzTime(std::mktime(&gmt));
}

bool SzCache::validatePathMember(std::string member) 
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

SzCache::SzPath SzCache::checkPath(SzPath path) 
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
		return SzPath(cSzCacheDir + std::string("/") + path);
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

bool SzCache::directoryExists(const SzPath& path)
{
	struct stat res;
	if(stat(path.c_str(), &res) != 0) {
		return false;
	}
	
	if (res.st_mode & S_IFDIR) 
		return true;
	
	return false;
}

bool SzCache::fileExists(const SzPath& path)
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


std::set<std::string> SzCache::globify(const SzPath& path)
{
	glob_t res;
	glob(path.c_str(), GLOB_TILDE, NULL, &res);
	std::set<std::string> s;
	for (int i=0; i < res.gl_pathc; ++i)
		s.insert(std::string(res.gl_pathv[i]));
		
	globfree(&res);
	return s;
}

SzCache::SzRange SzCache::searchFirstLast(SzPath path)
{
	std::set<std::string> s = globify(path + std::string("/[0-9][0-9][0-9][0-9][0-9][0-9]") + cSzCacheExt);
	if (s.size() == 0) return SzRange(SzTime(-1),SzTime(-1));
	
	std::set<std::string>::const_iterator beg = s.begin();
	std::set<std::string>::iterator end = beg;
	std::advance(end, s.size() - 1);
	return SzRange (getTime(0,*beg),getTime(-1,*end));
}

SzCache::SzTime SzCache::searchAt(SzTime start, SzPath path)
{
	SzPathIndex szpi = getPathIndex(start, path);
	std::cout << szpi.first;
	if (!fileExists(szpi.first)) 
		return SzTime(-1);

	szpi.second *= cSzCacheSize;
	if (getFileSize(szpi.first) < szpi.second)
		return SzTime(-1);

	SzCacheFile szf(szpi.first);
	try { 
		szf.open();
		if (szf[szpi.second] == cSzCacheNoData)
			return SzTime(-1);

	} catch (std::exception& e) {
		return SzTime(-1);
	}
	
	return start;
}


