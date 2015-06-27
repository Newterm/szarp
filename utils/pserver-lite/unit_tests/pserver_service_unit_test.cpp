
#include <cppunit/extensions/HelperMacros.h>

#include <vector>
#include <string>
#include "pserver_service.h"
#include "command_handler.h"

class PServerServiceTest : public CppUnit::TestFixture
{

CPPUNIT_TEST_SUITE(PServerServiceTest);
CPPUNIT_TEST(testParseArgs);
CPPUNIT_TEST(testOk);
CPPUNIT_TEST(testGetCmd);
CPPUNIT_TEST_SUITE_END();

private:
	std::vector<std::string> parse_test;
	std::vector<std::string> arg_test;
	std::vector<std::string> ok_test;
	std::vector<std::string> getcmd_test;
	PServerService pss;

public:
	void setUp (void) {
		/* testParseArgs() */
		// no CRLF
		parse_test.push_back(std::string("GET start end path"));
		// empty message
		parse_test.push_back(std::string("\r\n"));
		// whitespaces at the beginning
		parse_test.push_back(std::string(" SEARCH arg1 arg2 arg3\r\n"));

		// unknown command
		arg_test.push_back(std::string("PUT start end path\r\n"));
		arg_test.push_back(std::string("GETx 1435437865 1435437978 param_path\r\n"));

		/* testOk() */
		ok_test.push_back(std::string("GET   1435437865\t1435437978 param_path\r\n"));
		ok_test.push_back(std::string("GET 1435437865 1435437978 param_path\r\n"));
		ok_test.push_back(std::string("SEARCH start_time end_time direction param_path\r\n"));
		ok_test.push_back(std::string("SEARCH \tstart_time  end_time \t direction param_path\r\n"));
		ok_test.push_back(std::string("RANGE\r\n"));
		ok_test.push_back(std::string("RANGE  \r\n"));

		/* testGetCmd() */
		// too few arguments
		getcmd_test.push_back(std::string("GET 1435243589 param_path\r\n"));
		getcmd_test.push_back(std::string("GET \r\n"));
		// cannot cast to time_t
		getcmd_test.push_back(std::string("GET -5 1435243589 param_path\r\n"));
		getcmd_test.push_back(std::string("GET start 1435243589 param_path\r\n"));
		// start_time > end_time
		getcmd_test.push_back(std::string("GET 1435243687 1435243589 param_path\r\n"));
		// last argument is zero length
		getcmd_test.push_back(std::string("GET 1435243687 1435243589 \r\n"));
	}

	void tearDown (void) { }

	void testParseArgs (void) {
		for (auto &msg : parse_test) {
			CPPUNIT_ASSERT_THROW(pss.process_request(msg), CommandHandler::ParseError);
		}
		for (auto &msg : arg_test) {
			CPPUNIT_ASSERT_THROW(pss.process_request(msg), CommandHandler::ArgumentError);
		}
	}

	void testOk (void) {
		for (auto &msg : ok_test) {
			CPPUNIT_ASSERT_NO_THROW(pss.process_request(msg));
		}
	}

	void testGetCmd (void) {
		for (auto &msg : arg_test) {
			CPPUNIT_ASSERT_THROW(pss.process_request(msg), CommandHandler::ArgumentError);
		}
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(PServerServiceTest);
