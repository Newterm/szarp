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
#include "sz4/base.h"
#include "sz4/path.h"
#include "sz4/block_cache.h"
#include "sz4/block.h"
#include "sz4/buffer.h"
#include "sz4/util.h"
#include "sz4/definable_param_cache.h"
#include "sz4/real_param_entry.h"
#include "sz4/lua_optimized_param_entry.h"
#include "sz4/lua_param_entry.h"
#include "sz4/rpn_param_entry.h"
#include "sz4/buffer_templ.h"
#include "sz4/lua_interpreter_templ.h"

#include "test_serach_condition.h"
#include "test_observer.h"
#include "simple_mocks.h"
#include "sz4/filelock.h"

class Sz4RPNParam : public CPPUNIT_NS::TestFixture
{
	void test1();
	void test2();

	CPPUNIT_TEST_SUITE( Sz4RPNParam );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST( test2 );
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
};

namespace {

class IPKContainerMock1 : public mocks::IPKContainerMockBase {
	TSzarpConfig config;
	TParam* param;
public:
	IPKContainerMock1() : param(new TParam(NULL, &config, L"1", TParam::DEFINABLE, TParam::P_DEFINABLE)) {
		param->SetDataType(TParam::DOUBLE);
		param->SetName(L"A:B:C1");
		AddParam(param);

		m_config.AddDrawDefinable(param);
	}
	TParam* DoGetParam(const std::wstring&) { return param; }
};

}


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4RPNParam );

void Sz4RPNParam::setUp() {
}

namespace {

struct test_types {
	typedef IPKContainerMock1 ipk_container_type;
	typedef mocks::mock_param_factory param_factory;
};

}



void Sz4RPNParam::test1() {
	IPKContainerMock1 mock;
	sz4::base_templ<test_types> base(L"", &mock);
	sz4::buffer_templ<test_types>* buff = base.buffer_for_param(mock.GetParam(L""));

	sz4::weighted_sum<double, sz4::second_time_t> sum;
	sz4::weighted_sum<double, sz4::second_time_t>::time_diff_type weight;
	buff->get_weighted_sum(mock.GetParam(L""), 100u, 200u, PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(10., sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(10), weight);
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());
}

namespace rpn_unit_test {

class IPKContainerMock2 : public mocks::IPKContainerMockBase {
	TParam* param;
	TParam* param2;
public:
	IPKContainerMock2() : param(new TParam(NULL, NULL, std::wstring(), TParam::NONE, TParam::P_REAL)),
				param2(new TParam(NULL, NULL, L"(A:B:C) 1 +", TParam::DEFINABLE, TParam::P_DEFINABLE))
	 {
		param->SetDataType(TParam::SHORT);
		param->SetName(L"A:B:C");
		AddParam(param);
		m_config.AddDrawDefinable(param);

		param2->SetDataType(TParam::DOUBLE);
		param2->SetName(L"A:B:D");
		AddParam(param2);
		m_config.AddDrawDefinable(param2);

	}

	TParam* DoGetParam(const std::wstring& name) {
		if (name == L"BASE:A:B:C")
			return param;

		if (name == L"BASE:A:B:D")
			return param2;

		assert(false);
		return NULL;
	}
};

struct test_types {
	typedef IPKContainerMock2 ipk_container_type;
	typedef mocks::mock_param_factory param_factory;
};

}


void Sz4RPNParam::test2() {
	rpn_unit_test::IPKContainerMock2 mock;

	std::wstringstream base_dir_name;
	base_dir_name << L"/tmp/sz4_rpn_param" << getpid() << L"." << time(NULL) << L".tmp";
	boost::filesystem::wpath base_path(base_dir_name.str());
	boost::filesystem::wpath param_dir(base_path / L"BASE/sz4/A/B/C");
	boost::filesystem::create_directories(param_dir);

#if BOOST_FILESYSTEM_VERSION == 3
	sz4::base_templ<rpn_unit_test::test_types> base(base_path.wstring(), &mock);
#else
	sz4::base_templ<rpn_unit_test::test_types> base(base_path.file_string(), &mock);
#endif
	sz4::buffer_templ<rpn_unit_test::test_types>* buff = base.buffer_for_param(mock.GetParam(L"BASE:A:B:D"));

	
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
		std::string file_path_str;
#if BOOST_FILESYSTEM_VERSION == 3
		file_path_str = (param_dir / file_name.str()).native();
#else
		file_path_str = (param_dir / file_name.str()).external_file_string();
#endif
		int fd;
		CPPUNIT_ASSERT_NO_THROW(fd = sz4::open_writelock(file_path_str.c_str(), O_WRONLY | O_CREAT));

		char buf[6];
		short v = 10;
		sz4::second_time_t t = 150;

		memcpy(buf, &v, 2);
		memcpy(buf + 2, &t, 4);

		write(fd, buf, sizeof(buf));

		sz4::close_unlock(fd);
	}

	CPPUNIT_ASSERT(o.wait_for(1, 2));
	{
		sz4::weighted_sum<double, sz4::second_time_t> sum;
		sz4::weighted_sum<double, sz4::second_time_t>::time_diff_type weight;
		buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:D"), 100u, 200u, PT_SEC10, sum);
		CPPUNIT_ASSERT_EQUAL(55., sum.sum(weight));
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(5), weight);
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(5), sum.no_data_weight());
		CPPUNIT_ASSERT_EQUAL(false, sum.fixed());
	}
}

