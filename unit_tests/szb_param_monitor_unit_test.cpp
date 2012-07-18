#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cppunit/extensions/HelperMacros.h>
#include <boost/filesystem.hpp>

#include "conversion.h"

#include "szbase/szbparamobserver.h"
class TParam;
#include "szbase/szbparammonitor.h"

class SzbParamMonitorTest : public CPPUNIT_NS::TestFixture
{
	void writeTest();
	void moveTest();

	CPPUNIT_TEST_SUITE( SzbParamMonitorTest );
	CPPUNIT_TEST( writeTest );
	CPPUNIT_TEST( moveTest );
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
};

class TestObserver : public SzbParamObserver { 
public:
	std::map<TParam*, int> map;
	void param_data_changed(TParam* param) {
		std::map<TParam*, int>::iterator i = map.find(param);
		if (i == map.end())
			map[param] = 1;
		else
			i->second++;
	}
};


CPPUNIT_TEST_SUITE_REGISTRATION( SzbParamMonitorTest );

void SzbParamMonitorTest::setUp() {
}

void SzbParamMonitorTest::writeTest() {
	SzbParamMonitor m;
	TestObserver o;

	std::stringstream file_name;
	file_name << "/tmp/szb_monitor_unit_test_" << getpid() << "." << time(NULL) << ".tmp";
	
	boost::filesystem::create_directories(file_name.str());
	std::vector<std::string> dir_paths;
	for (char c = 'a'; c < 'd'; c++) {
		std::stringstream s;
		s << c;

		boost::filesystem::path p = boost::filesystem::path(file_name.str()) / s.str();
		boost::filesystem::create_directories(p);
		dir_paths.push_back(p.file_string());
	}

	m.add_observer(&o, std::vector<std::pair<TParam*, std::vector<std::string> > >(1, std::make_pair((TParam*)1, dir_paths)));
	sleep(1);

	for (std::vector<std::string>::iterator i = dir_paths.begin();
			i != dir_paths.end();
			i++) {

		std::string s = *i + "/test.sz4";
		std::ofstream ofs(s.c_str(), std::ios_base::binary);
		ofs.write("1111", 4);
	}

	sleep(1);
	CPPUNIT_ASSERT_EQUAL(1u, o.map.size());
	CPPUNIT_ASSERT_EQUAL(3, o.map[(TParam*)1]);
}

void SzbParamMonitorTest::moveTest() {
}

