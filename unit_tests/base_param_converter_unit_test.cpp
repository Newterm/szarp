#include "config.h"

#include <cppunit/extensions/HelperMacros.h>

#include "szarp_base_common/lua_param_optimizer.h"
#include "szarp_base_common/lua_param_optimizer_templ.h"

class BaseParamConverterTestCase : public CPPUNIT_NS::TestFixture
{
	void test1();

	CPPUNIT_TEST_SUITE( BaseParamConverterTestCase );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST_SUITE_END();
};

class IPKContainerMock {
public:
	TSzarpConfig* GetConfig(const std::wstring&) { return NULL; }
	TParam* GetParam(const std::wstring&) { return NULL; }
};

void BaseParamConverterTestCase::test1() {
	IPKContainerMock mock;
	LuaExec::ParamConverterTempl<IPKContainerMock> conv(&mock);
}
