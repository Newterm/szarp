#include <bzlib.h>

#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

void save_bz2_file(const std::vector<unsigned char>& data, const boost::filesystem::wpath& path) {
	auto dir = path.parent_path();
	auto tmp = dir / boost::filesystem::unique_path();
	
	BZFILE* b = BZ2_bzopen(tmp.native().c_str(), "wb");
	BZ2_bzwrite(b, const_cast<unsigned char*>(&data[0]), data.size());
	BZ2_bzclose(b);

	boost::filesystem::rename(tmp, path);
}
