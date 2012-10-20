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
#include "sz4/defs.h"
#include "sz4/time.h"
#include "sz4/path.h"
#include "sz4/block.h"
#include "sz4/buffer.h"
#include "sz4/definable_param_cache.h"
#include "szarp_base_common/lua_param_optimizer_templ.h"
#include "sz4/real_param_entry.h"
#include "sz4/lua_optimized_param_entry.h"
#include "sz4/buffer_templ.h"
#include "sz4/base.h"

#include "test_serach_condition.h"
#include "test_observer.h"

class Sz4LuaParamOptimized : public CPPUNIT_NS::TestFixture
{
	void test1();
	void test2();

	CPPUNIT_TEST_SUITE( Sz4LuaParamOptimized );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST( test2 );
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
};

namespace {

class IPKContainerMock1 {
	TParam param;
	TSzarpConfig config;
public:
	IPKContainerMock1() : param(NULL, NULL, std::wstring(), TParam::LUA_VA, TParam::P_LUA) {
		param.SetConfigId(0);
		param.SetParamId(0);
		param.SetDataType(TParam::DOUBLE);
		param.SetName(L"A:B:C1");
		param.SetLuaScript((const unsigned char*) 
"if (t % 100) < 50 then "
"	v = 0		"
"else			"
"	v = 1		"
"end			"
);
		param.SetParentSzarpConfig(&config);

		config.SetName(L"TEST", L"TEST");
	}
	TSzarpConfig* GetConfig(const std::wstring&) { return &config; }
	TParam* GetParam(const std::wstring&) { return &param; }
};

}


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4LuaParamOptimized );

void Sz4LuaParamOptimized::setUp() {
}

void Sz4LuaParamOptimized::test1() {
	IPKContainerMock1 mock;
	sz4::base_templ<IPKContainerMock1> base(L"", &mock);
	sz4::buffer_templ<IPKContainerMock1>* buff = base.buffer_for_param(mock.GetParam(L""));

	sz4::weighted_sum<double, sz4::second_time_t> sum;
	buff->get_weighted_sum(mock.GetParam(L""), 100u, 200u, PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(50., sum.sum());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(100), sum.weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());
}

namespace unit_test {

class IPKContainerMock2 {
	TParam param;
	TParam param2;
	TSzarpConfig config;
public:
	IPKContainerMock2() : param(NULL, NULL, std::wstring(), TParam::NONE, TParam::P_REAL),
				param2(NULL, NULL, std::wstring(), TParam::LUA_VA, TParam::P_LUA)
	 {
		param.SetConfigId(0);
		param.SetParamId(0);
		param.SetDataType(TParam::SHORT);
		param.SetName(L"A:B:C");
		param.SetParentSzarpConfig(&config);

		param2.SetConfigId(0);
		param2.SetParamId(1);
		param2.SetDataType(TParam::DOUBLE);
		param2.SetLuaScript((const unsigned char*) 
"v = p(\"BASE:A:B:C\", t, pt)"
);
		param2.SetParentSzarpConfig(&config);

		config.SetName(L"TEST", L"TEST");
	}

	TSzarpConfig* GetConfig(const std::wstring&) { return (TSzarpConfig*) 1; }

	TParam* GetParam(const std::wstring& name) {
		if (name == L"BASE:A:B:C")
			return &param;

		if (name == L"BASE:A:B:D")
			return &param2;
		
		assert(false);
		return NULL;
	}
};

}


void Sz4LuaParamOptimized::test2() {
	unit_test::IPKContainerMock2 mock;

	std::wstringstream base_dir_name;
	base_dir_name << L"/tmp/sz4_lua_param_optimized" << getpid() << L"." << time(NULL) << L".tmp";
	boost::filesystem::wpath base_path(base_dir_name.str());
	boost::filesystem::wpath param_dir(base_path / L"TEST/sz4/A/B/C");
	boost::filesystem::create_directories(param_dir);

	sz4::base_templ<unit_test::IPKContainerMock2> base(base_path.file_string(), &mock);
	sz4::buffer_templ<unit_test::IPKContainerMock2>* buff = base.buffer_for_param(mock.GetParam(L"BASE:A:B:D"));

	
	TestObserver o;
	base.param_monitor().add_observer(&o, mock.GetParam(L"BASE:A:B:C"), param_dir.external_file_string(), 10);
	{
		sz4::weighted_sum<double, sz4::second_time_t> sum;
		buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:D"), 100u, 200u, PT_SEC10, sum);
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.weight());
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(100), sum.no_data_weight());
		CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

		std::wstringstream file_name;
		file_name << std::setfill(L'0') << std::setw(10) << 100 << L".sz4";
		std::ofstream ofs((param_dir / file_name.str()).external_file_string().c_str());

		short v = 10;
		ofs.write((const char*)&v, sizeof(v));

		sz4::second_time_t t = 150;
		ofs.write((const char*)&t, sizeof(t));
	}

	CPPUNIT_ASSERT(o.wait_for(1, 2));
	{
		sz4::weighted_sum<double, sz4::second_time_t> sum;
		buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:D"), 100u, 200u, PT_SEC10, sum);
		CPPUNIT_ASSERT_EQUAL(10 * 50., sum.sum());
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.weight());
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
		CPPUNIT_ASSERT_EQUAL(false, sum.fixed());
	}
}

