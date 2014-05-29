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
#include "sz4/block_cache.h"
#include "sz4/block.h"
#include "sz4/path.h"
#include "sz4/block.h"
#include "sz4/buffer.h"
#include "sz4/base.h"
#include "sz4/load_file_locked.h"

class Sz4BaseTestCase : public CPPUNIT_NS::TestFixture
{
protected:
	std::wstring m_base_dir_name;

public:

	void smokeTest1();
	void cacheTest1();
	void setUp();
	void tearDown();

	CPPUNIT_TEST_SUITE( Sz4BaseTestCase );
	CPPUNIT_TEST( smokeTest1 );
	CPPUNIT_TEST( cacheTest1 );
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION( Sz4BaseTestCase );

void Sz4BaseTestCase::setUp() {
	std::wstringstream base_dir_name;
	base_dir_name << L"/tmp/sz4_base_test_1" << getpid() << L"." << time(NULL) << L".tmp";
	
	m_base_dir_name = base_dir_name.str();

	boost::filesystem::wpath path(m_base_dir_name);

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

#if BOOST_FILESYSTEM_VERSION == 3
		std::ofstream ofs((path / ls.str() / L"config/params.xml").native().c_str(), std::ios_base::binary);
#else
		std::ofstream ofs((path / ls.str() / L"config/params.xml").external_file_string().c_str(), std::ios_base::binary);
#endif
		ofs << 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<params xmlns=\"http://www.praterm.com.pl/SZARP/ipk\" version=\"1.0\" read_freq=\"10\" send_freq=\"10\" title=\"Globalmalt Bydgoszcz\" documentation_base_url=\"http://www.szarp.com\">"
"  <device xmlns:modbus=\"http://www.praterm.com.pl/SZARP/ipk-extra\" daemon=\"/dev/null\">"
"    <unit id=\"1\" type=\"1\" subtype=\"1\" bufsize=\"1\">"
"    	<param name=\"a:a:a\" short_name=\"-\" unit=\"°C\" prec=\"1\" draw_name=\"\" base_ind=\"auto\">"
"      	</param>"
"    	<param name=\"b:b:b\" short_name=\"-\" unit=\"°C\" prec=\"1\" draw_name=\"\" base_ind=\"auto\">"
"      	</param>"
"    	<param name=\"Status:Meaner4:Heartbeat\" short_name=\"-\" unit=\"°C\" prec=\"1\" draw_name=\"\" base_ind=\"auto\">"
"      	</param>"
"    </unit>"
"  </device>"
"  <drawdefinable>"
"    <param name=\"z:z:z\" short_name=\"\" draw_name=\"\" unit=\"%\" prec=\"0\" data_type=\"double\">"
"      <define type=\"LUA\" lua_formula=\"va\" start_date=\"-1\">"
"        <script><![CDATA["
"		v = p(\"a:a:a:a\", t, pt) + p(\"a:a:a:a\", t, pt)"
"	 ]]></script>"
"      </define>"
"    </param>"
"  </drawdefinable>"
"</params>";
		ofs.close();

		for (wchar_t s = L'a'; s <= L'b'; s++) {
			for (int i = 1000; i < 2000; i += 100) {
				std::wstringstream ss;
				ss << l;

				boost::filesystem::wpath param_path = path / ls.str() / L"sz4" / ss.str() / ss.str() / ss.str();
				std::wstringstream file_name;
				file_name << std::setfill(L'0') << std::setw(10) << i << L".sz4";
#if BOOST_FILESYSTEM_VERSION == 3
				std::ofstream ofs((param_path / file_name.str()).native().c_str(), std::ios_base::binary);
#else
				std::ofstream ofs((param_path / file_name.str()).external_file_string().c_str(), std::ios_base::binary);
#endif
				short value = 10;
				ofs.write((const char*) &value, sizeof(value));
				unsigned time = i + 50;
				ofs.write((const char*) &time, sizeof(time));
			}
		}
	}

	IPKContainer::Init(m_base_dir_name, L"/opt/szarp", L"pl");

}

void Sz4BaseTestCase::tearDown() {
	IPKContainer::Destroy();
	boost::filesystem::remove_all(m_base_dir_name);
}

void Sz4BaseTestCase::smokeTest1() {
	IPKContainer* ipk = IPKContainer::GetObject();

	TParam* p11 = ipk->GetParam(std::wstring(L"a:a:a:a"));
	TParam* p12 = ipk->GetParam(std::wstring(L"a:b:b:b"));
	TParam* p21 = ipk->GetParam(std::wstring(L"b:a:a:a"));
	TParam* p22 = ipk->GetParam(std::wstring(L"b:b:b:b"));

	CPPUNIT_ASSERT(p11 != NULL);
	CPPUNIT_ASSERT(p12 != NULL);
	CPPUNIT_ASSERT(p21 != NULL);
	CPPUNIT_ASSERT(p22 != NULL);

	sz4::base base(m_base_dir_name, ipk);

	sz4::weighted_sum<short, sz4::second_time_t> sum;
	sz4::weighted_sum<short, sz4::second_time_t>::time_diff_type weight;
	base.get_weighted_sum(p11, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<short>::type(100), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(10), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(10), sum.no_data_weight());

}

void Sz4BaseTestCase::cacheTest1() {
	IPKContainer* ipk = IPKContainer::GetObject();

	TParam* pr = ipk->GetParam(std::wstring(L"a:a:a:a"));
	CPPUNIT_ASSERT(pr != NULL);

	TParam* pl = ipk->GetParam(std::wstring(L"a:z:z:z"));
	CPPUNIT_ASSERT(pl != NULL);

	sz4::base base(m_base_dir_name, ipk);

	sz4::weighted_sum<short, sz4::second_time_t>::time_diff_type weight;

	sz4::weighted_sum<short, sz4::second_time_t> sum_r;
	base.get_weighted_sum(pr, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum_r);
	CPPUNIT_ASSERT_EQUAL(size_t(10), sum_r.refferred_blocks().size());

	sz4::weighted_sum<double, sz4::second_time_t> sum_l;
	base.get_weighted_sum(pl, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum_l);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(sz4::value_sum<double>::type(100), sum_l.sum(weight), 0.5);
	CPPUNIT_ASSERT_EQUAL(size_t(10), sum_l.refferred_blocks().size());

	size_t size_in_bytes, blocks_count;
	base.cache()->cache_size(size_in_bytes,blocks_count);
	CPPUNIT_ASSERT_EQUAL(size_t(11), blocks_count);

}

