#include <cppunit/extensions/HelperMacros.h>

#include "argsmgr.h"
#include "cmdlineparser.h"

class ArgsManagerTest: public CPPUNIT_NS::TestFixture
{
	ArgsManager args_mgr{"test"};

public:

	void test1();
	void test2();
	void setUp();
	void tearDown();

	CPPUNIT_TEST_SUITE( ArgsManagerTest );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST( test2 );
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION( ArgsManagerTest );

void ArgsManagerTest::setUp() {
	args_mgr.parse<DefaultArgs>("", )
}

void ArgsManagerTest::tearDown() {
	IPKContainer::Destroy();
	boost::filesystem::remove_all(m_base_dir_name);
}

void ArgsManagerTest::smokeTest1() {
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
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<short>::type(5000), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), sum.no_data_weight());

}

void ArgsManagerTest::cacheTest1() {
	IPKContainer* ipk = IPKContainer::GetObject();

	TParam* pr = ipk->GetParam(std::wstring(L"a:a:a:a"));
	CPPUNIT_ASSERT(pr != NULL);

	TParam* pl = ipk->GetParam(std::wstring(L"a:z:z:z"));
	CPPUNIT_ASSERT(pl != NULL);

	sz4::base base(m_base_dir_name, ipk);

	sz4::weighted_sum<short, sz4::second_time_t>::time_diff_type weight;

	sz4::weighted_sum<short, sz4::second_time_t> sum_r;
	base.get_weighted_sum(pr, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum_r);

	sz4::weighted_sum<double, sz4::second_time_t> sum_l;
	base.get_weighted_sum(pl, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum_l);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(double(1000), double(sum_l.sum(weight)), 0.5);

	size_t size_in_bytes, blocks_count;
	base.cache()->cache_size(size_in_bytes,blocks_count);
	CPPUNIT_ASSERT_EQUAL(size_t(12), blocks_count);

}

