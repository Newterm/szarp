#include <fstream>
#include <iostream>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "conversion.h"

namespace sz4 {

bool load_file_locked(const std::wstring& path, void *data, size_t size) {
	namespace bi = boost::interprocess;

	std::basic_string<unsigned char> upath = SC::S2U(path);

	bi::file_lock file_lock((const char*) upath.c_str());
	bi::sharable_lock<bi::file_lock> sharable_lock(file_lock);

	std::ifstream ifs((const char*)SC::S2U(path).c_str(), std::ios::binary | std::ios::in);
	ifs.read((char*) data, size);
	return size_t(ifs.gcount()) == size;
}

}
