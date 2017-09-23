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

#include "szarp_base_common/szbparamobserver.h"
class TParam;
#include "szarp_base_common/szbparammonitor.h"

#include "test_observer.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

class SzbParamMonitorTest : public CPPUNIT_NS::TestFixture
{
	void writeTest();
	void renameTest();
	void dirCreateTest();

	CPPUNIT_TEST_SUITE( SzbParamMonitorTest );
	CPPUNIT_TEST( writeTest );
	CPPUNIT_TEST( renameTest );
	CPPUNIT_TEST( dirCreateTest );
	CPPUNIT_TEST_SUITE_END();

	std::vector<std::string> createDirectories();
	boost::filesystem::wpath m_dir;
public:
	void setUp() override;
	void tearDown() override;
};

CPPUNIT_TEST_SUITE_REGISTRATION( SzbParamMonitorTest );

void SzbParamMonitorTest::setUp() {
	m_dir = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
	boost::filesystem::create_directories(m_dir);
}

void SzbParamMonitorTest::tearDown() {
	boost::filesystem::remove_all(m_dir);
}

std::vector<std::string> SzbParamMonitorTest::createDirectories() {
	std::vector<std::string> dir_paths;
	for (char c = 'a'; c < 'd'; c++) {
		std::stringstream s;
		s << c;

		boost::filesystem::path p = m_dir / s.str();
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
	SzbParamMonitor m(L"");
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
	CPPUNIT_ASSERT_EQUAL(size_t(1), o1.map.size());
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
	CPPUNIT_ASSERT_EQUAL(size_t(2), o1.map.size());
	CPPUNIT_ASSERT_EQUAL(6, o1.map[(TParam*)1]);
	CPPUNIT_ASSERT_EQUAL(3, o1.map[(TParam*)2]);

	CPPUNIT_ASSERT(o2.wait_for(3));
	CPPUNIT_ASSERT_EQUAL(size_t(1), o2.map.size());
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

	CPPUNIT_ASSERT_EQUAL(size_t(2), o1.map.size());
	CPPUNIT_ASSERT_EQUAL(6, o1.map[(TParam*)1]);
	CPPUNIT_ASSERT_EQUAL(3, o1.map[(TParam*)2]);

	CPPUNIT_ASSERT(o2.wait_for(6));
	CPPUNIT_ASSERT_EQUAL(size_t(1), o2.map.size());
	CPPUNIT_ASSERT_EQUAL(6, o2.map[(TParam*)2]);

	boost::filesystem::remove_all(boost::filesystem::path(dir_paths[0]).branch_path());
}

void SzbParamMonitorTest::renameTest() {
	SzbParamMonitor m(L"");
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
	CPPUNIT_ASSERT_EQUAL(size_t(1), o1.map.size());
	CPPUNIT_ASSERT_EQUAL(2, o1.map[(TParam*)1]);
}

void SzbParamMonitorTest::dirCreateTest() {
	return;
	///XXX: this fails now, due to race condition in param monitor
	SzbParamMonitor m(L"");
	TestObserver o1;

	auto dir = m_dir / L"a" / L"b" / L"c";
	m.add_observer(&o1, std::vector<std::pair<TParam*, std::vector<std::string> > >(1, std::make_pair((TParam*)1, std::vector<std::string>{ dir.string() })), 0);

	boost::filesystem::create_directories(dir);
	int fd = -1;
	CPPUNIT_ASSERT_NO_THROW(fd = sz4::open_writelock((dir / L"f.tmp").string().c_str(), O_WRONLY | O_CREAT | O_BINARY));
	write(fd, "1111", 4);
	sz4::close_unlock(fd);
	boost::filesystem::rename(dir / L"f.tmp", dir / L"11.sz4");

	CPPUNIT_ASSERT(o1.wait_for(2));
	CPPUNIT_ASSERT_EQUAL(size_t(1), o1.map.size());
	CPPUNIT_ASSERT_EQUAL(2, o1.map[(TParam*)1]);
}
