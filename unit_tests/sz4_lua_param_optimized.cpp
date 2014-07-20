#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <limits>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cppunit/extensions/HelperMacros.h>
#include <boost/filesystem.hpp>

#include "conversion.h"
#include "szarp_config.h"
#include "liblog.h"
#include "szarp_base_common/defines.h"
#include "szarp_base_common/lua_param_optimizer.h"
#include "szarp_base_common/lua_param_optimizer_templ.h"
#include "sz4/defs.h"
#include "sz4/time.h"
#include "sz4/path.h"
#include "sz4/base.h"
#include "sz4/block_cache.h"
#include "sz4/block.h"
#include "sz4/buffer.h"
#include "sz4/definable_param_cache.h"
#include "sz4/real_param_entry.h"
#include "sz4/lua_optimized_param_entry.h"
#include "sz4/lua_param_entry.h"
#include "sz4/rpn_param_entry.h"
#include "sz4/buffer_templ.h"
#include "sz4/lua_interpreter_templ.h"
#include "sz4/filelock.h"

#include "test_serach_condition.h"
#include "test_observer.h"
#include "simple_mocks.h"

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
public:
	void setUp();
	void tearDown();

	std::wstringstream base_dir_name;
	boost::filesystem::wpath base_path;
	boost::filesystem::wpath param_dir;
};

void Sz4LuaParamOptimized::setUp() {
	base_dir_name << L"/tmp/sz4_lua_param_optimized" << getpid() << L"." << time(NULL) << L".tmp";

	base_path = base_dir_name.str();
	param_dir = base_path / L"BASE/sz4/A/B/C";
	boost::filesystem::create_directories(param_dir);

}

void Sz4LuaParamOptimized::tearDown() {
	boost::filesystem::remove_all(base_path);
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
	sz4::buffer_templ<test_types>* buff = base.buffer_for_param(mock.GetParam(L""));

	sz4::weighted_sum<double, sz4::second_time_t> sum;
	sz4::weighted_sum<double, sz4::second_time_t>::time_diff_type weight;
	buff->get_weighted_sum(mock.GetParam(L""), 100u, 200u, PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(5., sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(10), weight);
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
	sz4::base_templ<test_types2> base(base_path.wstring(), &mock);
#else
	sz4::base_templ<test_types2> base(base_path.file_string(), &mock);
#endif
	sz4::buffer_templ<test_types2>* buff = base.buffer_for_param(mock.GetParam(L"BASE:A:B:D"));
	
	TestObserver o;
#if BOOST_FILESYSTEM_VERSION == 3
	base.param_monitor().add_observer(&o, mock.GetParam(L"BASE:A:B:C"), param_dir.native(), 10);
#else
	base.param_monitor().add_observer(&o, mock.GetParam(L"BASE:A:B:C"), param_dir.external_file_string(), 10);
#endif
	{
		sz4::weighted_sum<double, sz4::second_time_t> sum;
		buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:D"), 100u, 200u, PT_SEC10, sum);
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.weight());
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(1), sum.no_data_weight());
		CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

		std::wstringstream file_name;
		file_name << std::setfill(L'0') << std::setw(10) << 100 << L".sz4";

#if BOOST_FILESYSTEM_VERSION == 3
		std::string file_path((param_dir / file_name.str()).native().c_str());
#else
		std::string file_path((param_dir / file_name.str()).external_file_string().c_str());
#endif
		int fd = sz4::open_writelock(file_path.c_str(), O_WRONLY | O_CREAT);

		short v = 10;
		write(fd, (const char*)&v, sizeof(v));

		sz4::second_time_t t = 150;
		write(fd, (const char*)&t, sizeof(t));

		sz4::close_unlock(fd);
	}

	CPPUNIT_ASSERT(o.wait_for(1, 2));
	{
		sz4::weighted_sum<double, sz4::second_time_t> sum;
		sz4::weighted_sum<double, sz4::second_time_t>::time_diff_type weight;
		buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:D"), 100u, 200u, PT_SEC10, sum);
		CPPUNIT_ASSERT_EQUAL(50., sum.sum(weight));
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(5), weight);
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(5), sum.no_data_weight());
		CPPUNIT_ASSERT_EQUAL(false, sum.fixed());
	}
}

void Sz4LuaParamOptimized::test3() {
	IPKContainerMock2 mock;

#if BOOST_FILESYSTEM_VERSION == 3
	sz4::base_templ<test_types2> base(base_path.wstring(), &mock);
#else
	sz4::base_templ<test_types2> base(base_path.file_string(), &mock);
#endif
	sz4::buffer_templ<test_types2>* buff = base.buffer_for_param(mock.GetParam(L"BASE:A:B:E"));
	sz4::weighted_sum<double, sz4::second_time_t> sum;
	sz4::weighted_sum<double, sz4::second_time_t>::time_diff_type weight;
	buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:E"), 0u, 2u, PT_SEC, sum);
	CPPUNIT_ASSERT_EQUAL(3., sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(2), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());


	sz4::weighted_sum<double, sz4::nanosecond_time_t> sum_n;
	sz4::weighted_sum<double, sz4::nanosecond_time_t>::time_diff_type weight_n;
	buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:E"), sz4::nanosecond_time_t(0, 0), sz4::nanosecond_time_t(0, 500000000u), PT_HALFSEC, sum_n);

	CPPUNIT_ASSERT_EQUAL(1., sum_n.sum(weight_n));
	CPPUNIT_ASSERT_EQUAL(true, sum_n.fixed());

}
