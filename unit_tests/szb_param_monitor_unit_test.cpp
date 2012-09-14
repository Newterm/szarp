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
	void renameTest();

	CPPUNIT_TEST_SUITE( SzbParamMonitorTest );
	CPPUNIT_TEST( writeTest );
	CPPUNIT_TEST( renameTest );
	CPPUNIT_TEST_SUITE_END();

	std::vector<std::string> createDirectories();
public:
	void setUp();
};

class TestObserver : public SzbParamObserver { 
	boost::mutex m_mutex;
	boost::condition m_cond;
	size_t m_counter;
public:
	TestObserver();
	void reset_counter();
	std::map<TParam*, int> map;
	void param_data_changed(TParam* param, const std::string& path);
	bool wait_for(size_t events);
};

TestObserver::TestObserver() {
	m_counter = 0;
}

void TestObserver::param_data_changed(TParam* param, const std::string& path) {
	boost::mutex::scoped_lock lock(m_mutex);
	std::map<TParam*, int>::iterator i = map.find(param);
	if (i == map.end())
		map[param] = 1;
	else
		i->second++;
	m_counter += 1;
	m_cond.notify_one();
}

bool TestObserver::wait_for(size_t events) {
	boost::mutex::scoped_lock lock(m_mutex);
	while (events != m_counter)
		if (!m_cond.timed_wait(lock, boost::posix_time::seconds(5)))
			return false;
	return true;
}


CPPUNIT_TEST_SUITE_REGISTRATION( SzbParamMonitorTest );

void SzbParamMonitorTest::setUp() {
}

std::vector<std::string> SzbParamMonitorTest::createDirectories() {
	std::stringstream file_name;
	file_name << "/tmp/szb_monitor_unit_test_" << getpid() << "." << time(NULL) << ".tmp";
	std::vector<std::string> dir_paths;
	for (char c = 'a'; c < 'd'; c++) {
		std::stringstream s;
		s << c;

		boost::filesystem::path p = boost::filesystem::path(file_name.str()) / s.str();
		boost::filesystem::create_directories(p);
		dir_paths.push_back(p.file_string());
	}

	return dir_paths;
}

void SzbParamMonitorTest::writeTest() {
	SzbParamMonitor m;
	TestObserver o1, o2;

	std::vector<std::string> dir_paths = createDirectories();
	m.add_observer(&o1, std::vector<std::pair<TParam*, std::vector<std::string> > >(1, std::make_pair((TParam*)1, dir_paths)), 0);

	for (std::vector<std::string>::iterator i = dir_paths.begin();
			i != dir_paths.end();
			i++) {

		std::string s = *i + "/test.sz4";
		std::ofstream ofs(s.c_str(), std::ios_base::binary);
		ofs.write("1111", 4);
	}

	CPPUNIT_ASSERT(o1.wait_for(3));
	CPPUNIT_ASSERT_EQUAL(1u, o1.map.size());
	CPPUNIT_ASSERT_EQUAL(3, o1.map[(TParam*)1]);

	m.add_observer(&o1, std::vector<std::pair<TParam*, std::vector<std::string> > >(1, std::make_pair((TParam*)2, dir_paths)), 0);
	m.add_observer(&o2, std::vector<std::pair<TParam*, std::vector<std::string> > >(1, std::make_pair((TParam*)2, dir_paths)), 0);
	for (std::vector<std::string>::iterator i = dir_paths.begin();
			i != dir_paths.end();
			i++) {

		std::string s = *i + "/test.sz4";
		std::ofstream ofs(s.c_str(), std::ios_base::binary);
		ofs.write("1111", 4);
	}

	CPPUNIT_ASSERT(o1.wait_for(9));
	CPPUNIT_ASSERT_EQUAL(2u, o1.map.size());
	CPPUNIT_ASSERT_EQUAL(6, o1.map[(TParam*)1]);
	CPPUNIT_ASSERT_EQUAL(3, o1.map[(TParam*)2]);

	CPPUNIT_ASSERT(o2.wait_for(3));
	CPPUNIT_ASSERT_EQUAL(1u, o2.map.size());
	CPPUNIT_ASSERT_EQUAL(3, o2.map[(TParam*)2]);

	m.remove_observer(&o1);
	for (std::vector<std::string>::iterator i = dir_paths.begin();
			i != dir_paths.end();
			i++) {

		std::string s = *i + "/test.sz4";
		std::ofstream ofs(s.c_str(), std::ios_base::binary);
		ofs.write("1111", 4);
	}

	CPPUNIT_ASSERT_EQUAL(2u, o1.map.size());
	CPPUNIT_ASSERT_EQUAL(6, o1.map[(TParam*)1]);
	CPPUNIT_ASSERT_EQUAL(3, o1.map[(TParam*)2]);

	CPPUNIT_ASSERT(o2.wait_for(6));
	CPPUNIT_ASSERT_EQUAL(1u, o2.map.size());
	CPPUNIT_ASSERT_EQUAL(6, o2.map[(TParam*)2]);

	boost::filesystem::remove_all(boost::filesystem::path(dir_paths[0]).branch_path());
}

void SzbParamMonitorTest::renameTest() {
	SzbParamMonitor m;
	TestObserver o1;

	std::vector<std::string> dir_paths = createDirectories();
	m.add_observer(&o1, std::vector<std::pair<TParam*, std::vector<std::string> > >(1, std::make_pair((TParam*)1, dir_paths)), 0);
	{
		std::ofstream ofs((dir_paths[0] + "/f.tmp").c_str(), std::ios_base::binary);
		ofs.write("1111", 4);

		std::ofstream ofs2((dir_paths[1] + "/f.tmp").c_str(), std::ios_base::binary);
		ofs2.write("1111", 4);
	}

	boost::filesystem::rename(dir_paths[0] + "/f.tmp", dir_paths[0] + "/11.sz4");
	boost::filesystem::rename(dir_paths[1] + "/f.tmp", dir_paths[1] + "/11.sz4");

	CPPUNIT_ASSERT(o1.wait_for(2));
	CPPUNIT_ASSERT_EQUAL(1u, o1.map.size());
	CPPUNIT_ASSERT_EQUAL(2, o1.map[(TParam*)1]);
}

