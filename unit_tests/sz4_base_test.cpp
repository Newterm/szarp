#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <cppunit/extensions/HelperMacros.h>

#include "conversion.h"
#include "szarp_config.h"

#include "sz4/defs.h"
#include "sz4/time.h"
#include "sz4/block.h"
#include "sz4/path.h"
#include "sz4/block.h"
#include "sz4/buffer.h"
#include "sz4/base.h"
#include "sz4/load_file_locked.h"

class Sz4BaseTestCase : public CPPUNIT_NS::TestFixture
{
	void test1();

	CPPUNIT_TEST_SUITE( Sz4BaseTestCase );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION( Sz4BaseTestCase );

void Sz4BaseTestCase::test1() {
	std::wstringstream base_dir_name;
	base_dir_name << L"/tmp/sz4_base_test_1" << getpid() << L"." << time(NULL) << L".tmp";

	boost::filesystem::wpath path(base_dir_name.str());

	boost::filesystem::create_directories(path / L"b/config");
	boost::filesystem::create_directories(path / L"b/sz4");
	boost::filesystem::create_directories(path / L"b/sz4/a/a/a");
	boost::filesystem::create_directories(path / L"b/sz4/b/b/b");

	for (wchar_t l = L'a'; l < L'd'; l++)
	{
		std::wstringstream ls;
		ls << l;

		boost::filesystem::create_directories(path / ls.str() / L"config");
		boost::filesystem::create_directories(path / ls.str() / L"sz4");
		boost::filesystem::create_directories(path / ls.str() / L"sz4/a/a/a");
		boost::filesystem::create_directories(path / ls.str() / L"sz4/b/b/b");

		
		std::ofstream ofs((path / ls.str() / L"config/params.xml").external_file_string().c_str(), std::ios_base::binary);
		ofs << 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<params xmlns=\"http://www.praterm.com.pl/SZARP/ipk\" version=\"1.0\" read_freq=\"10\" send_freq=\"10\" title=\"Globalmalt Bydgoszcz\" documentation_base_url=\"http://www.szarp.com\">"
"  <device xmlns:modbus=\"http://www.praterm.com.pl/SZARP/ipk-extra\" daemon=\"/dev/null\">"
"    <unit id=\"1\" type=\"1\" subtype=\"1\" bufsize=\"1\">"
"    	<param name=\"a:a:a\" short_name=\"-\" unit=\"°C\" prec=\"1\" draw_name=\"\" base_ind=\"auto\">"
"      	</param>"
"    	<param name=\"b:b:b\" short_name=\"-\" unit=\"°C\" prec=\"1\" draw_name=\"\" base_ind=\"auto\">"
"      	</param>"
"    </unit>"
"  </device>"
"</params>";
		ofs.close();

		for (wchar_t s = L'a'; s <= L'b'; s++) {
			for (int i = 1000; i < 2000; i += 100) {
				std::wstringstream ss;
				ss << l;

				boost::filesystem::wpath param_path = path / ls.str() / L"sz4" / ss.str() / ss.str() / ss.str();
				std::wstringstream file_name;
				file_name << std::setfill(L'0') << std::setw(10) << i << L".sz4";
				std::ofstream ofs((param_path / file_name.str()).external_file_string().c_str(), std::ios_base::binary);
				short value = 10;
				ofs.write((const char*) &value, sizeof(value));
				unsigned time = i + 50;
				ofs.write((const char*) &time, sizeof(time));
			}
		}
	}

	IPKContainer::Init(base_dir_name.str(), L"/opt/szarp", L"pl");
	IPKContainer* ipk = IPKContainer::GetObject();

	TParam* p11 = ipk->GetParam(std::wstring(L"a:a:a:a"));
	TParam* p12 = ipk->GetParam(std::wstring(L"a:b:b:b"));
	TParam* p21 = ipk->GetParam(std::wstring(L"b:a:a:a"));
	TParam* p22 = ipk->GetParam(std::wstring(L"b:b:b:b"));

	CPPUNIT_ASSERT(p11 != NULL);
	CPPUNIT_ASSERT(p12 != NULL);
	CPPUNIT_ASSERT(p21 != NULL);
	CPPUNIT_ASSERT(p22 != NULL);

	sz4::base base(base_dir_name.str(), NULL);

	sz4::weighted_sum<short, sz4::second_time_t> sum;
	base.get_weighted_sum(p11, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<short>::type(5000), sum.sum());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), sum.weight());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), sum.no_data_weight());

	IPKContainer::Destroy();

	boost::filesystem::remove_all(base_dir_name.str());
}
