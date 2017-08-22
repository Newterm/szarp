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

#include "unit_test_common.h"

class Sz4BaseTestCase : public CPPUNIT_NS::TestFixture
{
protected:
	boost::filesystem::wpath m_base_dir;

public:

	void smokeTest1();
	void cacheTest1();
	void getLastTest();
	void setUp();
	void tearDown();

	CPPUNIT_TEST_SUITE( Sz4BaseTestCase );
	CPPUNIT_TEST( smokeTest1 );
	CPPUNIT_TEST( cacheTest1 );
	CPPUNIT_TEST( getLastTest );
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION( Sz4BaseTestCase );

void Sz4BaseTestCase::setUp() {
	m_base_dir = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
	boost::filesystem::create_directories(m_base_dir);	

	for (wchar_t l = L'a'; l < L'd'; l++)
	{
		std::wstringstream ls;
		ls << l;

		boost::filesystem::create_directories(m_base_dir / ls.str() / L"config");

#if BOOST_FILESYSTEM_VERSION == 3
		std::ofstream ofs((m_base_dir / ls.str() / L"config/params.xml").native().c_str(), std::ios_base::binary);
#else
		std::ofstream ofs((m_base_dir / ls.str() / L"config/params.xml").external_file_string().c_str(), std::ios_base::binary);
#endif
		ofs << 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<params xmlns=\"http://www.praterm.com.pl/SZARP/ipk\" version=\"1.0\" read_freq=\"10\" send_freq=\"10\" title=\"Globalmalt Bydgoszcz\" documentation_base_url=\"http://www.szarp.com\">"
"  <device xmlns:modbus=\"http://www.praterm.com.pl/SZARP/ipk-extra\" daemon=\"/dev/null\">"
"    <unit id=\"1\" type=\"1\" subtype=\"1\" bufsize=\"1\">"
"    	<param name=\"a:a:a\" data_type=\"uinteger\" short_name=\"-\" unit=\"°C\" prec=\"1\" draw_name=\"\" base_ind=\"auto\">"
"      	</param>"
"    	<param name=\"b:b:b\" data_type=\"uinteger\" short_name=\"-\" unit=\"°C\" prec=\"1\" draw_name=\"\" base_ind=\"auto\">"
"      	</param>"
"    	<param name=\"c:c:c\" data_type=\"ushort\" short_name=\"-\" unit=\"°C\" prec=\"1\" draw_name=\"\" base_ind=\"auto\">"
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

		for (wchar_t s = L'a'; s <= L'd'; s++) {
			for (int i = 1000; i < 2100; i += 100) {
				std::wstringstream ss;
				ss << s;

				boost::filesystem::wpath param_path;
				if (s != L'd')
					param_path = m_base_dir / ls.str() / L"szbase" / ss.str() / ss.str() / ss.str();
				else
					param_path = m_base_dir / ls.str() / L"szbase" / L"Status" / L"Meaner3" / L"program_uruchomiony";
				boost::filesystem::create_directories(param_path);

				std::ostringstream oss;

				if (s != L'd') {
					unsigned value = 10;
					ofs.write((const char*) &value, sizeof(value));

					unsigned char delta = 50;
					ofs.write((const char*) &delta, sizeof(delta));

					if (i == 2000)
					{
						value = 1;
						ofs.write((const char*) &value, sizeof(value));

						delta = 1;
						ofs.write((const char*) &delta, sizeof(delta));
					}	
				} else {

					short value = 10;
					ofs.write((const char*) &value, sizeof(value));

					unsigned char delta = 50;
					ofs.write((const char*) &delta, sizeof(delta));

					if (i == 2000)
					{
						value = 1;
						ofs.write((const char*) &value, sizeof(value));

						delta = 1;
						ofs.write((const char*) &delta, sizeof(delta));
					}	
				}

				auto s = oss.str();
				std::wstringstream file_name;
				file_name << std::setfill(L'0') << std::setw(10) << i << L".sz4";
				save_bz2_file({ s.begin(), s.end() }, param_path / file_name.str());
			}

		}
	}

	IPKContainer::Init(m_base_dir.wstring(), L"/opt/szarp", L"pl");

}

void Sz4BaseTestCase::tearDown() {
	IPKContainer::Destroy();
	boost::filesystem::remove_all(m_base_dir);
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

	sz4::base base(m_base_dir.wstring(), ipk);

	sz4::weighted_sum<unsigned , sz4::second_time_t> sum;
	sz4::weighted_sum<unsigned , sz4::second_time_t>::time_diff_type weight;
	base.get_weighted_sum(p11, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<unsigned>::type(5000), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), sum.no_data_weight());

}

void Sz4BaseTestCase::cacheTest1() {
	IPKContainer* ipk = IPKContainer::GetObject();

	TParam* pr = ipk->GetParam(std::wstring(L"a:a:a:a"));
	CPPUNIT_ASSERT(pr != NULL);

	TParam* pl = ipk->GetParam(std::wstring(L"a:z:z:z"));
	CPPUNIT_ASSERT(pl != NULL);

	sz4::base base(m_base_dir.wstring(), ipk);

	sz4::weighted_sum<unsigned, sz4::second_time_t>::time_diff_type weight;

	sz4::weighted_sum<unsigned, sz4::second_time_t> sum_r;
	base.get_weighted_sum(pr, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum_r);

	sz4::weighted_sum<double, sz4::second_time_t> sum_l;
	base.get_weighted_sum(pl, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum_l);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(double(1000), double(sum_l.sum(weight)), 0.5);

	size_t size_in_bytes, blocks_count;
	base.cache()->cache_size(size_in_bytes,blocks_count);
	CPPUNIT_ASSERT_EQUAL(size_t(12), blocks_count);

}

void Sz4BaseTestCase::getLastTest() {
	IPKContainer* ipk = IPKContainer::GetObject();

	TParam* pr = ipk->GetParam(std::wstring(L"a:a:a:a"));
	CPPUNIT_ASSERT(pr != NULL);

	TParam* pl = ipk->GetParam(std::wstring(L"a:z:z:z"));
	CPPUNIT_ASSERT(pl != NULL);

	sz4::base base(m_base_dir.wstring(), ipk);


	sz4::second_time_t tr;
	base.get_last_time(pr, tr);
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(2051), tr);

	//get_weighted_sum for any range starting at last time should yield no-data
	sz4::weighted_sum<unsigned, sz4::second_time_t> sum_0r;
	base.get_weighted_sum(pr, tr, sz4::time_just_after(tr), PT_SEC, sum_0r);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum_0r.weight());

	//get_weighted_sum for time just before last time should return sth
	sz4::weighted_sum<unsigned, sz4::second_time_t> sum_1r;
	base.get_weighted_sum(pr, sz4::time_just_before(tr), tr, PT_SEC, sum_1r);
	CPPUNIT_ASSERT_EQUAL(unsigned(1), sum_1r.avg());

	sz4::nanosecond_time_t trn;
	base.get_last_time(pr, trn);
	CPPUNIT_ASSERT_EQUAL(sz4::nanosecond_time_t(2051, 0), trn);

	//get_weighted_sum for any range starting at last time should yield no-data
	sz4::weighted_sum<unsigned, sz4::nanosecond_time_t> sum_0rn;
	base.get_weighted_sum(pr, trn, sz4::time_just_after(trn), PT_SEC, sum_0rn);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::nanosecond_time_t>::type(0), sum_0rn.weight());

	//get_weighted_sum for time just before last time should return sth
	sz4::weighted_sum<double, sz4::nanosecond_time_t> sum_1rn;
	base.get_weighted_sum(pr, sz4::time_just_before(trn), trn, PT_SEC, sum_1rn);
	CPPUNIT_ASSERT_EQUAL(double(1), sum_1rn.avg());

	//the same as before but for LUA param
	sz4::second_time_t tl;
	base.get_last_time(pl, tl);
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(2051), tl);

	sz4::weighted_sum<double, sz4::second_time_t> sum_0l;
	base.get_weighted_sum(pl, tl, sz4::time_just_after(tl), PT_SEC, sum_0l);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum_0l.weight());

	sz4::weighted_sum<double, sz4::second_time_t> sum_1l;
	base.get_weighted_sum(pl, sz4::time_just_before(tl), tl, PT_SEC, sum_1l);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.2, sum_1l.avg(), 0.05);

	sz4::nanosecond_time_t tln;
	base.get_last_time(pl, tln);
	CPPUNIT_ASSERT_EQUAL(sz4::nanosecond_time_t(2051, 0), tln);

	sz4::weighted_sum<double, sz4::nanosecond_time_t> sum_0ln;
	base.get_weighted_sum(pl, tln, sz4::time_just_after(tln), PT_SEC, sum_0ln);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::nanosecond_time_t>::type(0), sum_0ln.weight());

	sz4::weighted_sum<double, sz4::nanosecond_time_t> sum_1ln;
	base.get_weighted_sum(pl, sz4::time_just_before(tln), tln, PT_SEC, sum_1ln);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.2, sum_1ln.avg(), 0.05);
}
