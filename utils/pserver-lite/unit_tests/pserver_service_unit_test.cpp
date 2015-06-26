
#include <cppunit/extensions/HelperMacros.h>

#include <vector>
#include <string>
#include "pserver_service.h"
#include "command_handler.h"

class PServerServiceTest : public CppUnit::TestFixture
{

CPPUNIT_TEST_SUITE(PServerServiceTest);
CPPUNIT_TEST(testParse);
CPPUNIT_TEST_SUITE_END();

private:
	std::vector<std::string> parse_test;
	PServerService pss;

public:
	void setUp() {
		parse_test.push_back(std::string("GET start end path"));
		parse_test.push_back(std::string("\r\n"));
		parse_test.push_back(std::string(" SEARCH arg1 arg2 arg3\r\n"));
		parse_test.push_back(std::string("PUT start end path\r\n"));
	}

	void tearDown() { }

	void testParse()
	{
		for (auto &msg : parse_test) {
			CPPUNIT_ASSERT_THROW(pss.process_request(msg), CommandHandler::ParseError);
		}
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(PServerServiceTest);
