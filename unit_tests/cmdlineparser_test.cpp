#include <cppunit/extensions/HelperMacros.h>

#include "argsmgr.h"
#include "cmdlineparser.h"

class CmdLineParserTest: public CPPUNIT_NS::TestFixture
{
	CmdLineParser cmdparser{"test"};

public:

	void test1();
	void setUp();
	void tearDown();

	CPPUNIT_TEST_SUITE( CmdLineParserTest );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION( CmdLineParserTest );

void CmdLineParserTest::setUp() {}

void CmdLineParserTest::tearDown() {}

void CmdLineParserTest::test1() {
	char * arg1 = "/opt/szarp/bin/test";
	char * arg2 = "--base=testbase";
	char * arg3 = "-Dprefix=testprefix";

	char * cmd_line[] = {arg1, arg2, arg3};

	cmdparser.parse(3, (const char**)cmd_line, DefaultArgs());
	CPPUNIT_ASSERT(cmdparser.has("base"));
	CPPUNIT_ASSERT_EQUAL(*cmdparser.get<std::string>("base"), std::string("testbase"));
	CPPUNIT_ASSERT(!cmdparser.has("help"));

	// cmd doesnt parse overriden
	CPPUNIT_ASSERT(!((bool)cmdparser.get<std::string>("prefix")));
}
