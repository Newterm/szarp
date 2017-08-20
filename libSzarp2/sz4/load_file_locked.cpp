#include "config.h"

#include <fstream>
#include <iostream>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <sys/file.h>
#include <bzlib.h>

#include "conversion.h"

namespace sz4 {

bool load_file_locked(const boost::filesystem::wpath& path, std::vector<unsigned char>& data) {
	std::string path_string;
#if BOOST_FILESYSTEM_VERSION == 3
	path_string = path.string().c_str();
#else
	path_string = path.external_file_string();
#endif
	bool ret = false;

	BZFILE* bz2 = BZ2_bzopen(path_string.c_str(), "r+b");
	if (!bz2)
		return false;

	size_t read = 0;
	data.resize(1024);
	while (true) {
		int r, error;

		r = BZ2_bzRead(&error, bz2, &data[read], data.size() - read);
		if (error != BZ_STREAM_END && error != BZ_OK)
			break;

		read += r;
		if (error == BZ_STREAM_END) {
			ret = true;
			break;
		}

		data.resize(data.size() * 2);
	}

	data.resize(read);

	BZ2_bzclose(bz2);

	return ret;
}

}
