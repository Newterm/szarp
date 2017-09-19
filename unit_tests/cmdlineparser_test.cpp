#include <cppunit/extensions/HelperMacros.h>

#include "argsmgr.h"
#include "cmdlineparser.h"

class ArgsManagerTest: public CPPUNIT_NS::TestFixture
{
	CmdLineParser cmdparser{"test"};

public:

	void test1();
	void setUp();
	void tearDown();

	CPPUNIT_TEST_SUITE( ArgsManagerTest );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION( ArgsManagerTest );

void ArgsManagerTest::setUp() {}

void ArgsManagerTest::tearDown() {}

void ArgsManagerTest::test1() {
	char * arg1 = "/opt/szarp/bin/test";
	char * arg2 = "--base=testbase";
	char * arg3 = "-v";
	char * arg4 = "-Dprefix=testprefix";

	char * cmd_line[] = {arg1, arg2, arg3, arg4};

	cmdparser.parse(4, (const char**)cmd_line, DefaultArgs());
	CPPUNIT_ASSERT(cmdparser.has("base"));
	CPPUNIT_ASSERT_EQUAL(*cmdparser.get<std::string>("base"), std::string("testbase"));
	CPPUNIT_ASSERT(cmdparser.has("verbose"));
	CPPUNIT_ASSERT(!cmdparser.has("help"));
	CPPUNIT_ASSERT_EQUAL(*cmdparser.get<std::string>("prefix"), std::string("testprefix"));
}
