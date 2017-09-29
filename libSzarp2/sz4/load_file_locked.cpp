#include "config.h"

#include <fstream>
#include <iostream>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <sys/file.h>

#include "conversion.h"
#include "sz4/filelock.h"

namespace sz4 {

class read_file_locker {
	int m_file_fd;
public:
	read_file_locker(const boost::filesystem::wpath& path);
	~read_file_locker();
};

read_file_locker::read_file_locker(const boost::filesystem::wpath& path) : m_file_fd(-1) {
#if BOOST_FILESYSTEM_VERSION == 3
	m_file_fd = open_readlock(path.string().c_str(), O_RDONLY);
#else
	m_file_fd = open_readlock(path.external_file_string().c_str(), O_RDONLY);
#endif
}

read_file_locker::~read_file_locker() {
	if (m_file_fd != -1)
		close_unlock(m_file_fd);
}

bool load_file_locked(const boost::filesystem::wpath& path, void *data, size_t size) {
	try {
		read_file_locker file_lock(path);
#if BOOST_FILESYSTEM_VERSION == 3
		std::ifstream ifs(path.string().c_str(), std::ios::binary | std::ios::in);
#else
		std::ifstream ifs(path.external_file_string().c_str(), std::ios::binary | std::ios::in);
#endif
		ifs.read((char*) data, size);
		return size_t(ifs.gcount()) == size;
	} catch (read_file_locker&) {
		return false;	
	}
}

}
