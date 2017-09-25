#include <fstream>
#include <iostream>

#include <boost/filesystem/path.hpp>

#include "conversion.h"

#include "sz4/time.h"
#include "sz4/load_current_file.h"

namespace sz4
{

namespace
{

bool load_data(std::istream& is, std::vector<unsigned char> &data)
{
	auto pos = is.tellg();
	is.seekg(0, is.end);

	size_t len = is.tellg() - pos;

	is.seekg(pos, is.beg);
	data.resize(len);

	return bool(is.read(reinterpret_cast<char*>(&data[0]), len));
}

}

bool load_current_file(std::istream& is, second_time_t& t, std::vector<unsigned char> &data)
{
	if (!is.read(reinterpret_cast<char*>(&t), sizeof(t)))
		return false;

	return load_data(is, data);
}

bool load_current_file(std::istream& is, nanosecond_time_t& t, std::vector<unsigned char> &data)
{
	if (!is.read(reinterpret_cast<char*>(&t.second), sizeof(t.second)))
		return false;

	if (!is.read(reinterpret_cast<char*>(&t.nanosecond), sizeof(t.nanosecond)))
		return false;

	return load_data(is, data);
}

bool load_current_file(const boost::filesystem::wpath& path, second_time_t& t, std::vector<unsigned char> &data)
{
	std::ifstream f(path.string().c_str(), std::ifstream::binary);
	return load_current_file(f, t, data);
}

bool load_current_file(const boost::filesystem::wpath& path, nanosecond_time_t& t, std::vector<unsigned char> &data)
{
	std::ifstream f(path.string().c_str(), std::ifstream::binary);
	return load_current_file(f, t, data);
}


}
