#ifndef __SZB_CACHE_H__
#define __SZB_CACHE_H__

#include <memory>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

using boost::asio::ip::tcp;

class SzbCache : public std::enable_shared_from_this<SzbCache> {

public:
	class invalid_path_ex : public std::exception {};

	SzbCache( std::string& cache_dir );

        void get(int out_fd, std::string& param_path, time_t start, time_t end);
        void range(time_t* first, time_t* last);
        time_t search(time_t start, time_t end, int direction, std::string& param_path, time_t& first, time_t& last);

        void search_first_last(std::string& param_path, time_t& first, time_t& last);
        void search_first_last(fs::path& param_path, time_t& first, time_t& last);

	size_t get_block_size(time_t start, time_t end);


protected:

        void send_file(int out_fd, fs::path fp, off_t offset, size_t count);

        fs::path check_path(std::string& param_path);
        time_t search_at(time_t t, fs::path& full_path);

	time_t search_for(time_t start, time_t end, int direction, fs::path& full_path);

        time_t search_left(time_t start, time_t end, fs::path& full_path);
        time_t search_right(time_t start, time_t end, fs::path& full_path);

	off_t search_one_file(off_t offset, size_t lenght, int direction, fs::path& pfile);

        size_t max_probes(const int year, const int month);
        time_t probe_time(const int year, const int month, const int probe);

        bool not_szcache_file(fs::path& path);

        fs::path m_cache_dir;

};

#endif /* end of include guard: __SZB_CACHE_H__ */
