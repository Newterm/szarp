#include "config.h"

#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cppunit/extensions/HelperMacros.h>
#include <boost/filesystem.hpp>

#include "conversion.h"
#include "sz4/filelock.h"

#include "szbase/szbparamobserver.h"
class TParam;
#include "szbase/szbparammonitor.h"

#include "test_observer.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

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
#if BOOST_FILESYSTEM_VERSION == 3
		dir_paths.push_back(p.string());
#else
		dir_paths.push_back(p.file_string());
#endif
	}

	return dir_paths;
}

void SzbParamMonitorTest::writeTest() {
	SzbParamMonitor m;
	TestObserver o1, o2;
	int fd = -1;

	std::vector<std::string> dir_paths = createDirectories();
	m.add_observer(&o1, std::vector<std::pair<TParam*, std::vector<std::string> > >(1, std::make_pair((TParam*)1, dir_paths)), 0);

	for (std::vector<std::string>::iterator i = dir_paths.begin();
			i != dir_paths.end();
			i++) {

		std::string s = *i + "/test.sz4";
		CPPUNIT_ASSERT_NO_THROW(fd = sz4::open_writelock(s.c_str(), O_WRONLY | O_CREAT | O_BINARY));
		write(fd, "1111", 4);
		sz4::close_unlock(fd);
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
		CPPUNIT_ASSERT_NO_THROW(fd = sz4::open_writelock(s.c_str(), O_WRONLY | O_BINARY));
		write(fd, "1111", 4);
		sz4::close_unlock(fd);
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
		CPPUNIT_ASSERT_NO_THROW(fd = sz4::open_writelock(s.c_str(), O_WRONLY | O_BINARY));
		write(fd, "1111", 4);
		sz4::close_unlock(fd);
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
	int fd = -1;

	std::vector<std::string> dir_paths = createDirectories();
	m.add_observer(&o1, std::vector<std::pair<TParam*, std::vector<std::string> > >(1, std::make_pair((TParam*)1, dir_paths)), 0);

	CPPUNIT_ASSERT_NO_THROW(fd = sz4::open_writelock((dir_paths[0] + "/f.tmp").c_str(), O_WRONLY | O_CREAT | O_BINARY));
	write(fd, "1111", 4);
	sz4::close_unlock(fd);

	CPPUNIT_ASSERT_NO_THROW(fd = sz4::open_writelock((dir_paths[1] + "/f.tmp").c_str(), O_WRONLY | O_CREAT | O_BINARY));
	write(fd, "1111", 4);
	sz4::close_unlock(fd);

	boost::filesystem::rename(dir_paths[0] + "/f.tmp", dir_paths[0] + "/11.sz4");
	boost::filesystem::rename(dir_paths[1] + "/f.tmp", dir_paths[1] + "/11.sz4");

	CPPUNIT_ASSERT(o1.wait_for(2));
	CPPUNIT_ASSERT_EQUAL(1u, o1.map.size());
	CPPUNIT_ASSERT_EQUAL(2, o1.map[(TParam*)1]);
}

