#include "config.h"

#include <cppunit/extensions/HelperMacros.h>

#include "liblog.h"
#include "loghandlers.h"

class DummyLogHandler: public szlog::LogHandler {
public:
	mutable bool reinit_called = false;
	mutable bool msg_logged = false;
	mutable std::string _msg = "";

	void log(const std::string& msg, szlog::priority p = szlog::priority::info) const override { msg_logged = true; _msg = msg; }
	void log(const char* msg, szlog::priority p = szlog::priority::info) const override { log(std::string(msg), p); }

};


class LogHandlerTest : public CPPUNIT_NS::TestFixture
{
	void test();

	CPPUNIT_TEST_SUITE( LogHandlerTest );
	CPPUNIT_TEST( test );
	CPPUNIT_TEST_SUITE_END();
};

void LogHandlerTest::test() {
	auto dh = new DummyLogHandler();
	std::shared_ptr<DummyLogHandler> lh(dh);

	szlog::log().set_logger(lh);
	szlog::log().set_log_treshold(2);

	szlog::log() << szlog::info << "This should not be logged" << szlog::endl;

	szlog::log() << szlog::flush;

	CPPUNIT_ASSERT(!dh->msg_logged);

	sz_log(4, "This %s", "neither");

	CPPUNIT_ASSERT(!dh->msg_logged);

	std::string MSG = "This should!";
	szlog::log() << szlog::critical << MSG << szlog::flush;
	CPPUNIT_ASSERT(dh->msg_logged);
	CPPUNIT_ASSERT_EQUAL(dh->_msg, MSG);

	dh->msg_logged = false;
	MSG = "This too!";
	sz_log(1, "This %s!", "too");
	szlog::log() << szlog::flush;
	CPPUNIT_ASSERT(dh->msg_logged);
	CPPUNIT_ASSERT_EQUAL(dh->_msg, MSG);
}

CPPUNIT_TEST_SUITE_REGISTRATION( LogHandlerTest );
