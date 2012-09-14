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
#include "sz4/load_file_locked.h"

class Sz4BufferTestCase : public CPPUNIT_NS::TestFixture
{
	void test1();
	void test2();

	CPPUNIT_TEST_SUITE( Sz4BufferTestCase );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST_SUITE_END();
public:
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
		data = i + 50;
		ofs.write((const char*) &data, sizeof(data));
	}


	TParam param(NULL);
	param.SetParamId(0);
	param.SetName(L"A:A:A");
	param.SetTimeType(TParam::SECOND);
	param.SetDataType(TParam::INT);

	SzbParamMonitor monitor;
	sz4::buffer* buffer = new sz4::buffer(&monitor, base_dir_name.str());
	sz4::weighted_sum<int, sz4::second_time_t> sum;

	buffer->get_weighted_sum(&param, sz4::second_time_t(1000), sz4::second_time_t(2000), sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(5000), sum.sum());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), sum.weight());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), sum.no_data_weight());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(0), sz4::second_time_t(1000), sum);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.weight());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(1000), sum.no_data_weight());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(2000), sz4::second_time_t(2050), sum);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.weight());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1050), sz4::second_time_t(1100), sum);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.weight());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1050), sz4::second_time_t(1150), sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(10 * 50), sum.sum());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.weight());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1025), sz4::second_time_t(1175), sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(10 * 75), sum.sum());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(75), sum.weight());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(75), sum.no_data_weight());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1025), sz4::second_time_t(1025), sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(0), sum.sum());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.weight());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1075), sz4::second_time_t(1075), sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(0), sum.sum());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.weight());
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());

	sleep(100000);
	delete buffer;
	boost::filesystem::remove_all(boost::filesystem::wpath(base_dir_name.str()));
}

void Sz4BufferTestCase::test2() {
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
		data = i + 50;
		ofs.write((const char*) &data, sizeof(data));
	}


	TParam param(NULL);
	param.SetParamId(0);
	param.SetName(L"A:A:A");
	param.SetTimeType(TParam::SECOND);
	param.SetDataType(TParam::INT);

	SzbParamMonitor monitor;
	sz4::buffer buffer(&monitor, base_dir_name.str());
	sz4::weighted_sum<int, sz4::second_time_t> sum;

	boost::filesystem::remove_all(boost::filesystem::wpath(base_dir_name.str()));

}
