#include "config.h"

#include <cppunit/extensions/HelperMacros.h>

#include "liblog.h"
#include "loghandlers.h"
#include <fstream>
#include <unistd.h>
#include <stdio.h>


class FileloggerTest : public CPPUNIT_NS::TestFixture
{
	void test();

	CPPUNIT_TEST_SUITE( FileloggerTest );
	CPPUNIT_TEST( test );
	CPPUNIT_TEST_SUITE_END();
};

void FileloggerTest::test() {
	const std::string LOGFILE = "/tmp/test.log";
	truncate(LOGFILE.c_str(), (size_t) 0);

	szlog::log().set_logger<szlog::FileLogger>(LOGFILE);
	szlog::log() << szlog::critical << "0" << szlog::flush;

	if ((fork()) > 0) {
		/* parent */
		sz_log(0, "%d", 1);
	} else {
		/* child */
		exit(0);
	}

	szlog::log() << szlog::flush;

	std::ifstream logfile(LOGFILE);
	std::string date, hour, level, msg;
	logfile >> date >> hour >> level >> msg;
	CPPUNIT_ASSERT_EQUAL(msg, std::string("0"));

	logfile >> date >> hour >> level >> msg;
	CPPUNIT_ASSERT(msg == std::string("1"));

	CPPUNIT_ASSERT_EQUAL(remove("/tmp/test.log"), 0);
}

CPPUNIT_TEST_SUITE_REGISTRATION( FileloggerTest );
