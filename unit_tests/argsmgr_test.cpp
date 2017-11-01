#include <cppunit/extensions/HelperMacros.h>

#include "argsmgr.h"

class ArgsManagerTest: public CPPUNIT_NS::TestFixture
{
	ArgsManager args_mgr{"dummy"};

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
	char * arg3 = "--debug=10";
	char * arg4 = "-Dprefix=testprefix";

	char * cmd_line[] = {arg1, arg2, arg3, arg4};

	args_mgr.parse(4, (const char**)cmd_line, DefaultArgs());
	libpar_init_with_filename("libpar_test.cfg", 0);

	CPPUNIT_ASSERT(args_mgr.has("base"));
	CPPUNIT_ASSERT_EQUAL(*args_mgr.get<std::string>("base"), std::string("testbase"));
	CPPUNIT_ASSERT_EQUAL(*args_mgr.get<unsigned int>("debug"), (unsigned)10);
	CPPUNIT_ASSERT(!args_mgr.has("help"));
	CPPUNIT_ASSERT_EQUAL(*args_mgr.get<std::string>("prefix"), std::string("testprefix"));
	CPPUNIT_ASSERT(!((bool)args_mgr.get<std::string>("testpar")));
	CPPUNIT_ASSERT_EQUAL(*args_mgr.get<std::string>("testsect", "testpar"), std::string("testval"));
}
