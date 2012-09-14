#include "config.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <sys/file.h>

#include "conversion.h"

namespace sz4 {

class read_file_locker {
	int m_file_fd;
public:
	class file_lock_error : public std::runtime_error {
	public:
		file_lock_error(const std::string& error) : std::runtime_error(error) {}
	};
	read_file_locker(const boost::filesystem::wpath& path);
	~read_file_locker();
};

read_file_locker::read_file_locker(const boost::filesystem::wpath& path) : m_file_fd(-1) {
	m_file_fd = open(path.external_file_string().c_str(), O_RDONLY);
	if (m_file_fd == -1)
		throw file_lock_error("Failed to open file: " + path.external_file_string());

	if (flock(m_file_fd, LOCK_SH)) {
		close(m_file_fd);
		m_file_fd = -1;

		throw file_lock_error("Failed to lock file " + path.external_file_string());
	}
}

read_file_locker::~read_file_locker() {
	if (m_file_fd != -1) {
		flock(m_file_fd, LOCK_UN);
		close(m_file_fd);
	}
}

bool load_file_locked(const boost::filesystem::wpath& path, void *data, size_t size) {
	try {
		read_file_locker file_lock(path);
		std::ifstream ifs(path.external_file_string().c_str(), std::ios::binary | std::ios::in);
		ifs.read((char*) data, size);
		return size_t(ifs.gcount()) == size;
	} catch (read_file_locker&) {
		return false;	
	}
}

}
