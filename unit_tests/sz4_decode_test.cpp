#include "config.h"

#include <cppunit/extensions/HelperMacros.h>

#include "sz4/defs.h"
#include "sz4/decode_file.h"

class Sz4DecodeTEst : public CPPUNIT_NS::TestFixture
{
	void test();

	CPPUNIT_TEST_SUITE( Sz4DecodeTEst );
	CPPUNIT_TEST( test );
	CPPUNIT_TEST_SUITE_END();
};

void Sz4DecodeTEst::test() {
	//const unsigned char buf[] = { 0xE2,  0x0E, 0xAC, 0xE8 };
	const unsigned char buf[] = { 0xE2,  0x06, 0x2b, 0x90 };

	sz4::second_time_t t(0);
	CPPUNIT_ASSERT_EQUAL(size_t(4), sz4::decode_time(t, buf, 4));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(33958800), t);
}

CPPUNIT_TEST_SUITE_REGISTRATION( Sz4DecodeTEst );
