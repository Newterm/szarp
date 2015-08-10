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
#include "liblog.h"

#include "szarp_base_common/defines.h"
#include "szarp_base_common/lua_param_optimizer.h"
#include "szarp_base_common/lua_param_optimizer_templ.h"
#include "sz4/defs.h"
#include "sz4/block.h"
#include "sz4/block_cache.h"
#include "sz4/lua_interpreter_templ.h"

#include "sz4/time.h"
#include "sz4/block.h"
#include "sz4/path.h"
#include "sz4/base.h"
#include "sz4/buffer.h"
#include "sz4/real_param_entry.h"
#include "sz4/lua_optimized_param_entry.h"
#include "sz4/lua_param_entry.h"
#include "sz4/rpn_param_entry.h"
#include "sz4/buffer_templ.h"
#include "sz4/load_file_locked.h"

#include "test_observer.h"
#include "test_serach_condition.h"
#include "simple_mocks.h"

class Sz4BufferTestCase : public CPPUNIT_NS::TestFixture
{
	void test1();
	void test2();
	void searchTest();

	CPPUNIT_TEST_SUITE( Sz4BufferTestCase );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST( test2 );
	CPPUNIT_TEST( searchTest );
	CPPUNIT_TEST_SUITE_END();

	mocks::IPKContainerMock m_mock;
	sz4::base_templ<mocks::mock_types> m_base;
public:
	Sz4BufferTestCase() : m_base(L"", &m_mock) {}
	void setUp();
};


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4BufferTestCase );

void Sz4BufferTestCase::setUp() {
}

void Sz4BufferTestCase::test1() {
	std::wstringstream base_dir_name;
	base_dir_name << L"/tmp/szb_bufer_unit_test_1" << getpid() << L"." << time(NULL) << L".tmp";
	std::wstringstream param_dir_name;
	param_dir_name << base_dir_name.str() << L"/A/A/A";
	boost::filesystem::create_directories(param_dir_name.str());

	for (int i = 1000; i < 2000; i += 100) {
		std::wstringstream ss;
		ss << param_dir_name.str() << L"/" << std::setfill(L'0') << std::setw(10) << i << L".sz4";
		std::ofstream ofs(SC::S2A(ss.str()).c_str(), std::ios_base::binary);
		unsigned data = 10;
		ofs.write((const char*) &data, sizeof(data));
		unsigned char delta = 50;
		ofs.write((const char*) &delta, sizeof(delta));
	}


	TParam param(NULL);
	param.SetParamId(0);
	param.SetName(L"A:A:A");
	param.SetTimeType(TParam::SECOND);
	param.SetDataType(TParam::INT);

	SzbParamMonitor monitor;
	mocks::IPKContainerMock container_mock;
	container_mock.add_param(&param);
	sz4::buffer_templ<mocks::mock_types>* buffer = new sz4::buffer_templ<mocks::mock_types>(&m_base, &monitor, &container_mock, L"TEST", base_dir_name.str());
	sz4::weighted_sum<int, sz4::second_time_t> sum;
	sz4::weighted_sum<int, sz4::second_time_t>::time_diff_type weight;

	buffer->get_weighted_sum(&param, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(5000), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(0), sz4::second_time_t(1000), PT_SEC10, sum);
	sum.sum(weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(1000), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(2000), sz4::second_time_t(2050), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1050), sz4::second_time_t(1100), PT_SEC10, sum);
	sum.sum(weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1050), sz4::second_time_t(1150), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1025), sz4::second_time_t(1175), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(30 * 25), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(75), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(75), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1025), sz4::second_time_t(1025), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(0), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1075), sz4::second_time_t(1075), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(0), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	delete buffer;
	boost::filesystem::remove_all(boost::filesystem::wpath(base_dir_name.str()));
}

void Sz4BufferTestCase::test2() {
	std::wstringstream base_dir_name;
	base_dir_name << L"/tmp/szb_bufer_unit_test_2" << getpid() << L"." << time(NULL) << L".tmp";
	std::wstringstream param_dir_name;
	param_dir_name << base_dir_name.str() << L"/A/A/A";
	boost::filesystem::create_directories(param_dir_name.str());

	for (int i = 1000; i < 2000; i += 100) {
		std::wstringstream ss;
		ss << param_dir_name.str() << L"/" << std::setfill(L'0') << std::setw(10) << i << L".sz4";
		std::ofstream ofs(SC::S2A(ss.str()).c_str(), std::ios_base::binary);
		unsigned data = 10;
		ofs.write((const char*) &data, sizeof(data));
		unsigned char delta = 50;
		ofs.write((const char*) &delta, sizeof(delta));
	}


	TParam param(NULL);
	param.SetName(L"A:A:A");
	param.SetTimeType(TParam::SECOND);
	param.SetDataType(TParam::INT);

	SzbParamMonitor monitor;
	mocks::IPKContainerMock container_mock;
	container_mock.add_param(&param);
	sz4::buffer_templ<mocks::mock_types> buffer(&m_base, &monitor, &container_mock, L"TEST", base_dir_name.str());
	sz4::weighted_sum<int, sz4::second_time_t> sum;
	sz4::weighted_sum<int, sz4::second_time_t>::time_diff_type weight;

	buffer.get_weighted_sum(&param, sz4::second_time_t(1900), sz4::second_time_t(2000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

	TestObserver o;
	monitor.add_observer(&o, &param, SC::S2A(param_dir_name.str()), 1);
	{
		std::wstringstream ss;
		ss << param_dir_name.str() << L"/" << std::setfill(L'0') << std::setw(10) << 1900 << L".sz4";
		std::ofstream ofs(SC::S2A(ss.str()).c_str(), std::ios_base::binary | std::ios_base::app);
		unsigned data = 20;
		ofs.write((const char*) &data, sizeof(data));
		unsigned char delta = 50;
		ofs.write((const char*) &delta, sizeof(delta));
	}
	o.wait_for(1);

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer.get_weighted_sum(&param, sz4::second_time_t(1900), sz4::second_time_t(2000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(1500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(100), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	{
		std::wstringstream ss;
		ss << param_dir_name.str() << L"/" << std::setfill(L'0') << std::setw(10) << 1900 << L".sz4";
		std::ofstream ofs(SC::S2A(ss.str()).c_str(), std::ios_base::binary | std::ios_base::app);
		unsigned data = 30;
		ofs.write((const char*) &data, sizeof(data));

		//3000;
		unsigned char delta[] = { 0x83u, 0xe8u };
		ofs.write((const char*) delta, sizeof(delta));
	}
	o.wait_for(2);

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer.get_weighted_sum(&param, sz4::second_time_t(1900), sz4::second_time_t(3000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(31500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(1100), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer.get_weighted_sum(&param, sz4::second_time_t(1900), sz4::second_time_t(4000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

	boost::filesystem::remove_all(boost::filesystem::wpath(base_dir_name.str()));

}

void Sz4BufferTestCase::searchTest() {
	std::wstringstream base_dir_name;
	base_dir_name << L"/tmp/szb_bufer_unit_test_2" << getpid() << L"." << time(NULL) << L".tmp";
	std::wstringstream param_dir_name;
	param_dir_name << base_dir_name.str() << L"/A/A/A";
	boost::filesystem::create_directories(param_dir_name.str());

	for (int i = 0; i < 2100; i += 100) {
		std::wstringstream ss;
		ss << param_dir_name.str() << L"/" << std::setfill(L'0') << std::setw(10) << i << L".sz4";
		std::ofstream ofs(SC::S2A(ss.str()).c_str(), std::ios_base::binary);
		int j;
		for (j = 0; j < 100 - (2000 - i) / 100 - 10; j++) {
			unsigned data = j % 25;
			ofs.write((const char*) &data, sizeof(data));
			unsigned char delta = 1;
			ofs.write((const char*) &delta, sizeof(delta));
		}

		unsigned data = i / 100 + 10000;
		ofs.write((const char*) &data, sizeof(data));
		unsigned char delta = 1;
		ofs.write((const char*) &delta, sizeof(delta));
	}


	TParam param(NULL);
	param.SetName(L"A:A:A");
	param.SetTimeType(TParam::SECOND);
	param.SetDataType(TParam::INT);

	
	SzbParamMonitor monitor;
	mocks::IPKContainerMock container_mock;
	container_mock.add_param(&param);
	sz4::buffer_templ<mocks::mock_types> buffer(&m_base, &monitor, &container_mock, L"TEST", base_dir_name.str());

	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(0), buffer.search_data_right(&param, sz4::second_time_t(0), sz4::second_time_t(2000), PT_SEC10, test_search_condition(0)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(100), buffer.search_data_right(&param, sz4::second_time_t(100), sz4::second_time_t(2000), PT_SEC10, test_search_condition(0)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(125), buffer.search_data_right(&param, sz4::second_time_t(101), sz4::second_time_t(2000), PT_SEC10, test_search_condition(0)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(1), buffer.search_data_right(&param, sz4::second_time_t(0), sz4::second_time_t(2000), PT_SEC10, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(101), buffer.search_data_right(&param, sz4::second_time_t(100), sz4::second_time_t(2000), PT_SEC10, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(224), buffer.search_data_right(&param, sz4::second_time_t(190), sz4::second_time_t(2000), PT_SEC10, test_search_condition(24)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(224), buffer.search_data_right(&param, sz4::second_time_t(190), sz4::second_time_t(2000), PT_SEC10, test_search_condition(24)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, buffer.search_data_right(&param, sz4::second_time_t(10000), sz4::second_time_t(20000), PT_SEC10, test_search_condition(181)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, buffer.search_data_right(&param, sz4::second_time_t(1885), sz4::second_time_t(1900), PT_SEC10, test_search_condition(0)));
	for (int i = 0; i < 20; i++) {
		CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(100 * i + 70 + i), buffer.search_data_right(&param, sz4::second_time_t(0), sz4::second_time_t(2000), PT_SEC10, test_search_condition(10000 + i)));
	}
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, buffer.search_data_right(&param, sz4::second_time_t(182), sz4::second_time_t(2000), PT_SEC10, test_search_condition(181)));

	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(2000), buffer.search_data_left(&param, sz4::second_time_t(2000), sz4::second_time_t(0), PT_SEC10, test_search_condition(0)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(150), buffer.search_data_left(&param, sz4::second_time_t(198), sz4::second_time_t(100), PT_SEC10, test_search_condition(0)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, buffer.search_data_left(&param, sz4::second_time_t(1999), sz4::second_time_t(1998), PT_SEC10, test_search_condition(0)));
	for (int i = 0; i < 20; i++) {
		CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(100 * i + 70 + i), buffer.search_data_left(&param, sz4::second_time_t(2000), sz4::second_time_t(0), PT_SEC10, test_search_condition(10000 + i)));
	}

	boost::filesystem::remove_all(boost::filesystem::wpath(base_dir_name.str()));

}
