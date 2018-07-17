#include <cppunit/extensions/HelperMacros.h>
#include "conversion.h"

class ConversionTest: public CPPUNIT_NS::TestFixture
{
public:
	void test1();

	CPPUNIT_TEST_SUITE( ConversionTest );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST_SUITE_END();
};

void ConversionTest::test1() {
	CPPUNIT_ASSERT_EQUAL(SC::S2A(L"aąbcćdeęfghijklłmnńoóprsśtuwxyzźżAĄBCĆDEĘFGHIJKLŁMNŃOÓPRSŚTUWXYZŹŻ"), std::string("aabccdeefghijkllmnnooprstuwxyzzzAABCCDEEFGHIJKLLMNNOOPRSTUWXYZZZ"));

	unsigned char tt[] = u8"aąbcćdeęfghijklłmnńoóprsśtuwxyzźżAĄBCĆDEĘFGHIJKLŁMNŃOÓPRSŚTUWXYZŹŻ";

	CPPUNIT_ASSERT_EQUAL(SC::S2U(L"aąbcćdeęfghijklłmnńoóprsśtuwxyzźżAĄBCĆDEĘFGHIJKLŁMNŃOÓPRSŚTUWXYZŹŻ"), std::basic_string<unsigned char>(tt));
}
