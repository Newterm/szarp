#include "unit_test_common.h"

class Sz4LuaParamOptimized : public CPPUNIT_NS::TestFixture
{
	void test1();
	void test2();
	void test3();

	CPPUNIT_TEST_SUITE( Sz4LuaParamOptimized );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST( test2 );
	CPPUNIT_TEST( test3 );
	CPPUNIT_TEST_SUITE_END();

	boost::filesystem::wpath m_base_dir;
	boost::filesystem::wpath m_param_dir;
public:
	void setUp();
	void tearDown();
};

void Sz4LuaParamOptimized::setUp() {
	m_base_dir = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
	m_param_dir = m_base_dir / L"BASE" / L"szbase" / L"A" / L"B" / L"C";
	boost::filesystem::create_directories(m_param_dir);
}

void Sz4LuaParamOptimized::tearDown() {
	boost::filesystem::remove_all(m_base_dir);
}

namespace {

class IPKContainerMock1 : public mocks::IPKContainerMockBase {
	TParam param;
public:
	IPKContainerMock1() : param(NULL, NULL, std::wstring(), TParam::LUA_VA, TParam::P_LUA) {
		param.SetName(L"A:B:C1");
		param.SetLuaScript((const unsigned char*) 
"if (t % 100) < 50 then "
"	v = 0		"
"else			"
"	v = 1		"
"end			"
);
		AddParam(&param);
	}
	TParam* DoGetParam(const std::wstring&) { return &param; }
};

}


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4LuaParamOptimized );

namespace {

struct test_types {
	typedef IPKContainerMock1 ipk_container_type;
	typedef mocks::mock_param_factory param_factory;
};

}



void Sz4LuaParamOptimized::test1() {
	IPKContainerMock1 mock;
	sz4::base_templ<test_types> base(L"", &mock);
	auto buff = base.buffer_for_param(mock.GetParam(L""));

	sz4::weighted_sum<double, sz4::second_time_t> sum;
	sz4::weighted_sum<double, sz4::second_time_t>::time_diff_type weight;
	buff->get_weighted_sum(mock.GetParam(L""), 100u, 200u, PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(50., double(sum.sum(weight)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(100), weight);
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());
}

namespace {

class IPKContainerMock2 : public mocks::IPKContainerMockBase {
	TParam param;
	TParam param2;
	TParam param4;
public:
	IPKContainerMock2() : param(NULL, NULL, std::wstring(), TParam::NONE, TParam::P_REAL),
				param2(NULL, NULL, std::wstring(), TParam::LUA_VA, TParam::P_LUA),
				param4(NULL, NULL, std::wstring(), TParam::NONE, TParam::P_LUA)
	 {
		param.SetDataType(TParam::SHORT);
		param.SetName(L"A:B:C");
		AddParam(&param);

		param2.SetDataType(TParam::DOUBLE);
		param2.SetName(L"A:B:D");
		param2.SetLuaScript((const unsigned char*) 
"v = p(\"BASE:A:B:C\", t, pt)"
);
		AddParam(&param2);

		param4.SetDataType(TParam::DOUBLE);
		param4.SetName(L"A:B:E");
		param4.SetLuaScript((const unsigned char*) 
"if t > 0 then"
"	v = 2 * p(\"BASE:A:B:E\", 0, pt) "
"else"
"	v = 1 "
"end"
);
		AddParam(&param4);

	}

	TParam* DoGetParam(const std::wstring& name) {
		if (name == L"BASE:A:B:C")
			return &param;

		if (name == L"BASE:A:B:D")
			return &param2;

		if (name == L"BASE:A:B:E")
			return &param4;

		assert(false);
		return NULL;
	}

};

struct test_types2 {
	typedef IPKContainerMock2 ipk_container_type;
	typedef mocks::mock_param_factory param_factory;
};

}

void Sz4LuaParamOptimized::test2() {
	IPKContainerMock2 mock;

#if BOOST_FILESYSTEM_VERSION == 3
	sz4::base_templ<test_types2> base(m_base_dir.wstring(), &mock);
#else
	sz4::base_templ<test_types2> base(m_base_Dir.file_string(), &mock);
#endif
	auto buff = base.buffer_for_param(mock.GetParam(L"BASE:A:B:D"));
	
	TestObserver o;
#if BOOST_FILESYSTEM_VERSION == 3
	base.param_monitor().add_observer(&o, mock.GetParam(L"BASE:A:B:C"), m_param_dir.native(), 10);
#else
	base.param_monitor().add_observer(&o, mock.GetParam(L"BASE:A:B:C"), m_param_dir.external_file_string(), 10);
#endif
	{
		sz4::weighted_sum<double, sz4::second_time_t> sum;
		buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:D"), 100u, 200u, PT_SEC10, sum);
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.weight());
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(100), sum.no_data_weight());
		CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

		std::wstringstream file_name;
		file_name << std::setfill(L'0') << std::setw(10) << 100 << L".sz4";

		char buf[3];
		short v = 10;
		unsigned char t = 0x32;

		memcpy(buf, &v, 2);
		memcpy(buf + 2, &t, 1);

		save_bz2_file({buf, buf + 3}, m_param_dir / file_name.str());
	}

	CPPUNIT_ASSERT(o.wait_for(1, 2));
	{
		sz4::weighted_sum<double, sz4::second_time_t> sum;
		sz4::weighted_sum<double, sz4::second_time_t>::time_diff_type weight;
		buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:D"), 100u, 200u, PT_SEC10, sum);
		CPPUNIT_ASSERT_EQUAL(500., double(sum.sum(weight)));
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), weight);
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
		CPPUNIT_ASSERT_EQUAL(false, sum.fixed());
	}
}

void Sz4LuaParamOptimized::test3() {
	IPKContainerMock2 mock;

#if BOOST_FILESYSTEM_VERSION == 3
	sz4::base_templ<test_types2> base(m_base_dir.wstring(), &mock);
#else
	sz4::base_templ<test_types2> base(m_base_dir.file_string(), &mock);
#endif
	auto buff = base.buffer_for_param(mock.GetParam(L"BASE:A:B:E"));
	sz4::weighted_sum<double, sz4::second_time_t> sum;
	sz4::weighted_sum<double, sz4::second_time_t>::time_diff_type weight;
	buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:E"), 0u, 2u, PT_SEC, sum);
	CPPUNIT_ASSERT_EQUAL(3., double(sum.sum(weight)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(2), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());


	sz4::weighted_sum<double, sz4::nanosecond_time_t> sum_n;
	sz4::weighted_sum<double, sz4::nanosecond_time_t>::time_diff_type weight_n;
	buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:E"), sz4::nanosecond_time_t(0, 0), sz4::nanosecond_time_t(0, 500000000u), PT_HALFSEC, sum_n);

	CPPUNIT_ASSERT_EQUAL(1000000000., double(sum_n.sum(weight_n)));
	CPPUNIT_ASSERT_EQUAL(true, sum_n.fixed());

}
