
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#include <cerrno>
#include <cstring>

#include <fstream>
#include <boost/format.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>

#include "szbase/szbdate.h"
#include "szbcache.h"
#include "liblog.h"

using boost::format;

SzbCache::SzbCache( std::string& cache_dir ) : m_cache_dir(cache_dir)
{

};


void SzbCache::get(int out_fd, std::string& param_path, time_t start, time_t end)
{
	fs::path full_path = check_path(param_path);
	sz_log(10, "SzbCache::get check_path %s", full_path.string().c_str());
	
	if (!fs::is_directory(full_path)) {
		return;
	}
	
	int year, month;
	szb_time2my(start, &year, &month);
	off_t offset = szb_probeind(start, 10) * sizeof(SZB_FILE_TYPE);
	
	int end_year, end_month;
	szb_time2my(end, &end_year, &end_month);
	off_t end_off = szb_probeind(end, 10) * sizeof(SZB_FILE_TYPE);

	sz_log(10, "SzbCache::get y: %d, m: %d, e_y: %d, e_m: %d, p: %s",
			year, month, end_year, end_month, full_path.string().c_str());
        
        while (year <= end_year && month <= end_month) {
		sz_log(10, "SzbCache::get sending data from y: %d, m: %d", year, month);
		fs::path fp = full_path / str( format("%4d%02d.szc") % year % month );

		size_t count ;
		if (year == end_year && month == end_month) {
			count = end_off - offset + sizeof(SZB_FILE_TYPE);
		}
		else {
			count = fs::file_size(fp) - offset + sizeof(SZB_FILE_TYPE);
		}

		if (count != static_cast<uintmax_t>(-1) && count > 0) {
			send_file(out_fd, fp, offset, count);
		}
		
		if (month == 12) {
			year++;
			month = 1;
		}
		else {
			month++;
		}
		offset = 0;
	}
}

void SzbCache::send_file(int out_fd, fs::path fp, off_t offset, size_t count)
{
	const char * pfname = fp.string().c_str();
	sz_log(10, "SzbCache::send_file: pf: %s, off: %ld, cnt: %ld",
			pfname, offset, count);
	int fd = open(pfname, O_RDONLY | O_NOATIME);
	if (offset) {
		off_t sr = lseek(fd, offset, SEEK_SET);
		if ((off_t) -1 == sr) {
			sz_log(0, "SzbCache::send_file cannot lseek in file: %s", pfname);
			return;
		}
	}
	ssize_t ret = sendfile(out_fd, fd, NULL, count);
	sz_log(8, "SzbCache::send_file sendfile returned %ld", ret);
	close(fd);
}

void SzbCache::range(time_t* first, time_t* last)
{
        time(last);
	*last -= *last % 10;

        using namespace boost::posix_time;
        namespace bg = boost::gregorian;
        static ptime epoch(bg::date(1970, 1, 1));

        bg::date first_date = bg::day_clock::universal_day() - bg::months(3);
        bg::date first_day(first_date.year(), first_date.month(), 1);

        *first = time_t((ptime(first_day, seconds(0)) - epoch).total_seconds());
}

time_t SzbCache::search(time_t start, time_t end, int direction, std::string& param_path, time_t& first, time_t& last)
{

        fs::path full_path = check_path(param_path);
	sz_log(10, "SzbCache::search check_path %s", full_path.string().c_str());

        if (!fs::is_directory(full_path)) {
                sz_log(5, "SzbCache::search %s not directory", full_path.c_str());
                start = -1;
                end = -1;
                return -1;
        }

	sz_log(10, "SzbCache::search_first_last f: %ld, l: %ld",
			first, last);
        search_first_last(full_path, first, last);

        if ( 0 == direction ) {
                if ( -1 == start )
                        start = first;
                return search_at(start, full_path);
        }

        if ( direction > 0 ) {
                if ( -1 == start ||  start < first )
                        start = first;
                if ( -1 == end || end > last)
                        end = last;
        }
        else {
                if ( -1 == start || start > last )
                        start = last;
                if ( -1 == end || end < first )
                        end = first;
        }

	sz_log(10, "SzbCache::search_for");
	time_t found = search_for(start, end, direction, full_path);

        return found;
}

time_t SzbCache::search_at(time_t t, fs::path& full_path)
{
        int year, month;
        szb_time2my(t, &year, &month);

        int ind = szb_probeind(t, 10);

        fs::path fp = full_path / str( format("%4d%02d.szc") % year % month );
        
        SZB_FILE_TYPE val;
        try {
                std::ifstream ifs;
                ifs.exceptions(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);
                ifs.open(fp.string().c_str(), std::ios::binary | std::ios::in);
                ifs.seekg(sizeof(SZB_FILE_TYPE) * ind, std::ifstream::beg);
                ifs.read(reinterpret_cast<char*>(&val), sizeof(val));
                ifs.close();

                if (val == SZB_FILE_NODATA) {
                        return -1;
                }
                else {
                        return t;
                }
        }
        catch (std::exception& e) {
                sz_log(0, "SzbCache::exception: %s", e.what());
                return -1;
        }
}


time_t SzbCache::search_for(time_t start, time_t end, int direction, fs::path& full_path)
{
	switch (direction) {
		case -1:
			return search_left(start, end, full_path);
		case 1:
			return search_right(start, end, full_path);
		default:
			return -1;
	}
}

time_t SzbCache::search_left(time_t start, time_t end, fs::path& full_path)
{
	int y, m, ye, me;
	off_t ind;
	size_t lenght;
	time_t found = -1;

	time_t t = end; 

        sz_log(8, "SzbCache::search_left s: %ld e: %ld fp: %s", start, end, full_path.string().c_str());
	szb_time2my(start, &ye, &me);
	off_t inde = szb_probeind(start, 10);

	while (t > start) {
		szb_time2my(t, &y, &m);

		if (y == ye && m == me) {
			lenght = szb_probeind(t, 10);
			ind = inde;
		}
		else {
			lenght = szb_probeind(t, 10);
			ind = 0;
		}

		fs::path fp = full_path / str( format("%4d%02d.szc") % y % m );

		sz_log(9, "SzbCache::search_left y: %d, m: %d, ind: %ld, len: %ld, fp: %s", y, m, ind, lenght, fp.string().c_str());
		found = search_one_file(ind, lenght, -1, fp);
		if (found != -1)
			return t * ((found - ind) * 10);

		t -= (lenght * 10);
	}

	return -1;
}

time_t SzbCache::search_right(time_t start, time_t end, fs::path& full_path)
{
	int y, m, ye, me;
	off_t ind;
	size_t lenght;
	time_t found = -1;

	time_t t = start; 
        sz_log(8, "SzbCache::search_right s: %ld e: %ld fp: %s", start, end, full_path.string().c_str());

	szb_time2my(end, &ye, &me);
	//off_t inde = szb_probeind(end, 10);

	while (t < end) {
		szb_time2my(t, &y, &m);
		ind = szb_probeind(t, 10);

		if (y == ye && m == me)
			lenght = szb_probeind(end, 10);
		else
			lenght = max_probes(y, m);

		fs::path fp = full_path / str( format("%4d%02d.szc") % y % m );

		lenght = std::min(lenght, fs::file_size(fp) / sizeof(SZB_FILE_TYPE));

		if (ind > lenght)
			return -1;

		sz_log(9, "SzbCache::search_right y: %d, m: %d, ind: %ld, len: %ld, fp: %s",
			       	y, m, ind, lenght, fp.string().c_str());
		found = search_one_file(ind, lenght, 1, fp);
		sz_log(9, "SzbCache::search_right search_one_file found: %ld", found);
		if (found != -1)
			return t + ((found + ind) * 10);

		t += (max_probes(y, m) * 10);
	}

	return -1;
}

off_t SzbCache::search_one_file(off_t offset, size_t lenght, int direction, fs::path& pfile)
{
	sz_log(10, "SzbCache::search_one_file: %ld %ld %d %s",
			offset, lenght, direction, pfile.string().c_str());
	off_t pg_off = (offset * sizeof(SZB_FILE_TYPE)) & ~(sysconf(_SC_PAGE_SIZE) - 1);
	size_t pg_len = (lenght + offset) * sizeof(SZB_FILE_TYPE) - pg_off;
        int fd = open(pfile.string().c_str(), O_RDONLY | O_NOATIME);
        if (-1 == fd)
                return false;

        void* mdata = mmap(NULL, pg_len, PROT_READ, MAP_SHARED, fd, pg_off);
	short* data = (short*)(((char *)mdata) + (offset * sizeof(SZB_FILE_TYPE) - pg_off));
        if (MAP_FAILED == data) {
		sz_log(2, "SzbCache::search_one_file mmap error: %s", std::strerror(errno));
                close(fd);
                return -1;
        }
        
        off_t found = -1;
        if (direction > 0) {
                for (size_t pos = 0; pos < lenght; pos++) {
			if (data[pos] != SZB_FILE_NODATA) {
                                found = pos;
                                break;
                        }
                }
        }
        else {
                for (int pos = lenght - 1; pos >= 0; pos--) {
                        if (data[pos] != SZB_FILE_NODATA) {
                                found = pos;
                                break;
                        }
                }
        }

        munmap(mdata, lenght);
        close(fd);

        return found;
}

void SzbCache::search_first_last(fs::path& param_path, time_t& first, time_t& last)
{
        std::string sfirst;
        std::string slast;

        fs::directory_iterator end_itr;
        for (auto it = fs::directory_iterator(param_path); it != end_itr; it++) {
                fs::path p = it->path();
                if (not_szcache_file(p))
                        continue;

                std::string fn = p.stem().string();

                if (sfirst.empty() || sfirst > fn)
                        sfirst = fn;

                if (slast.empty() || slast < fn)
                        slast = fn;

        }
	sz_log(10, "SzbCache::search_first_last after iterator, sf: %s, sl %s", sfirst.c_str(), slast.c_str());

	int y = std::stoi(sfirst.substr(0, 4));
	int m = std::stoi(sfirst.substr(4, 2));
	first = probe_time(y, m, 0);
	
	y = std::stoi(slast.substr(0, 4));
	m = std::stoi(slast.substr(4, 2));
	last = probe_time(y, m, max_probes(y, m));

        sz_log(9, "SzbCache::search_first_last first: %ld last: %ld", first, last);
}

void SzbCache::search_first_last(std::string& param_path, time_t& first, time_t& last)
{
        fs::path full_path = check_path(param_path);

        if (!fs::is_directory(full_path)) {
                sz_log(5, "SzbCache::search %s not directory", full_path.string().c_str());
                first = -1;
                last = -1;
		return;
        }

        search_first_last(full_path, first, last);
}

size_t SzbCache::get_block_size(time_t start, time_t end)
{
	return ((end - start) / 10 + 1) * sizeof(SZB_FILE_TYPE);
}

bool SzbCache::not_szcache_file(fs::path& path)
{

        if (path.filename().extension() != ".szc")
                return true;

        const std::string& sstem = path.stem().string();
        for (auto it = sstem.begin(); it != sstem.end() && *it != '\0'; it++) {
                if (!std::isdigit(*it))
                        return true;
        }


        return false;
}

fs::path SzbCache::check_path(std::string& param_path)
{
        std::vector<std::string> parts;
        boost::split(parts, param_path, boost::is_any_of("/"));


        for (auto it = parts.begin(); it != parts.end(); it++) {
                if (it->empty() || *it == "." || *it == "..")
                        throw invalid_path_ex();
        }

        return m_cache_dir / fs::path(param_path);
}

size_t SzbCache::max_probes(const int year, const int month)
{
    static int probes[14] = { 0,
		    31 * 24 * 60 * 6,
		    28 * 24 * 60 * 6,
		    31 * 24 * 60 * 6,
		    30 * 24 * 60 * 6,
		    31 * 24 * 60 * 6,
		    30 * 24 * 60 * 6,
		    31 * 24 * 60 * 6,
		    31 * 24 * 60 * 6,
		    30 * 24 * 60 * 6,
		    31 * 24 * 60 * 6,
		    30 * 24 * 60 * 6,
		    31 * 24 * 60 * 6,
		    0 };
    
    if ((year < SZBASE_MIN_YEAR) || (year > SZBASE_MAX_YEAR))
	return 0;

    if (month == 2) {
	if ((year % 400) == 0)
	    return 29 * 24 * 60 * 6;
	if ((year % 100) == 0)
	    return 28 * 24 * 60 * 6;
	if ((year % 4) == 0)
	    return 29 * 24 * 60 * 6;
    }

    return probes[month];
}

time_t SzbCache::probe_time(const int year, const int month, const int probe)
{
	struct tm tm;
	
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = 1;
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;

	time_t t = timegm(&tm);
	
	return t + (probe * 10);
}

