#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cppunit/extensions/HelperMacros.h>
#include <boost/filesystem.hpp>

#include "conversion.h"

#include "sz4/path.h"
#include "sz4/block_for_date.h"

class Sz4FileSearchTestCase : public CPPUNIT_NS::TestFixture
{
	void test();

	CPPUNIT_TEST_SUITE( Sz4FileSearchTestCase );
	CPPUNIT_TEST( test );
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
};


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4FileSearchTestCase );

void Sz4FileSearchTestCase::setUp() {
}

std::wstring get_10d_number(const std::wstring& file_name) {
	return file_name.substr(file_name.size() - 14, 10);
}

void Sz4FileSearchTestCase::test() {
	std::wstringstream dir_name;
	dir_name << L"/tmp/szb_date_search_unit_test_" << getpid() << L"." << time(NULL) << L".tmp";
	boost::filesystem::create_directories(dir_name.str());

	for (int i = 1; i < 200; i += 2) {
		std::wstringstream ss;
		ss << dir_name.str() << L"/" << std::setfill(L'0') << std::setw(10) << i << L".sz4";
		std::ofstream ofs(SC::S2A(ss.str()).c_str(), std::ios_base::binary);
		ofs.write("aaa", 3);
	}

	CPPUNIT_ASSERT(sz4::find_block_for_date(dir_name.str(), 0u) == std::wstring());
	CPPUNIT_ASSERT(get_10d_number(sz4::find_block_for_date(dir_name.str(), 1u)) == std::wstring(L"0000000001"));
	CPPUNIT_ASSERT(get_10d_number(sz4::find_block_for_date(dir_name.str(), 10u)) == std::wstring(L"0000000009"));
	CPPUNIT_ASSERT(get_10d_number(sz4::find_block_for_date(dir_name.str(), 11u)) == std::wstring(L"0000000011"));
	CPPUNIT_ASSERT(get_10d_number(sz4::find_block_for_date(dir_name.str(), 200u)) == std::wstring(L"0000000199"));

	boost::filesystem::remove_all(dir_name.str());
}
