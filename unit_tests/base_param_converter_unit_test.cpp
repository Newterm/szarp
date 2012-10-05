#include "config.h"

#include "conversion.h"

#include <cppunit/extensions/HelperMacros.h>

#include "szarp_base_common/lua_param_optimizer.h"
#define LUA_OPTIMIZER_DEBUG
#include "szarp_base_common/lua_param_optimizer_templ.h"

class BaseParamConverterTestCase : public CPPUNIT_NS::TestFixture
{
	void test1();
	void test2();

	CPPUNIT_TEST_SUITE( BaseParamConverterTestCase );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST( test2 );
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION( BaseParamConverterTestCase );

class IPKContainerMock {
	TParam param;
public:
	IPKContainerMock() : param(NULL) {}
	TSzarpConfig* GetConfig(const std::wstring&) { return (TSzarpConfig*) 1; }
	TParam* GetParam(const std::wstring&) { return &param; }
};

void BaseParamConverterTestCase::test1() {
	IPKContainerMock mock;
	LuaExec::Param param;

	lua_grammar::chunk param_code;
	std::wstring f = L"v = 10";
	std::wstring::const_iterator param_text_begin = f.begin();
	std::wstring::const_iterator param_text_end = f.end();
	CPPUNIT_ASSERT(lua_grammar::parse(param_text_begin, param_text_end, param_code) && param_text_begin == param_text_end);

	LuaExec::ParamConverterTempl<IPKContainerMock> conv(&mock);
	try {
		conv.ConvertParam(param_code, &param);
	} catch (LuaExec::ParamConversionException& e) {
		std::cerr << (const char*) SC::S2U(e.what()).c_str() << std::endl;
		CPPUNIT_ASSERT(false);
	}
}

void BaseParamConverterTestCase::test2() {
	return;
	IPKContainerMock mock;
	LuaExec::Param param;

	lua_grammar::chunk param_code;
	
	std::string fu = 

"        local t0 = szb_move_time(t, -24, PT_HOUR, 0)"
"        local s"
"        local c = 0"
"        while t0 < t do"
"                local v0 = p(\"leg3:Kazimierza Wielkiego 26:Sterownik:temperatura zewnętrzna\", t0, PT_MIN10, 0)"
"                if not isnan(v0) then"
"                        if c == 0 then"
"                                s = v0"
"                                c = 1"
"                        else"
"                                s = s + v0"
"                                c = c + 1"
"                        end"
"                end"
"                t0 = szb_move_time(t0, 1, PT_MIN10, 0)"
"        end"
"        if c > 0 then"
"                local lv = s / c"
"                if lv > 6 then"
"                        v = 0"
"                elseif lv < 4.5 then"
"                        v = 1"
"                else"
"                        v = p(\"leg3:Kazimierza Wielkiego 26:Sterownik:tryb algorytmu 3\", szb_move_time(t, -1, PT_MIN10, 0), pt)"
"                end"
"        else"
"                v = nan()"
"        end"
"";

	std::wstring f = SC::U2S((const unsigned char*)fu.c_str());
	std::wstring::const_iterator param_text_begin = f.begin();
	std::wstring::const_iterator param_text_end = f.end();
	CPPUNIT_ASSERT(lua_grammar::parse(param_text_begin, param_text_end, param_code) && param_text_begin == param_text_end);

	LuaExec::ParamConverterTempl<IPKContainerMock> conv(&mock);
	try {
		conv.ConvertParam(param_code, &param);
	} catch (LuaExec::ParamConversionException& e) {
		std::cerr << (const char*) SC::S2U(e.what()).c_str() << std::endl;
		CPPUNIT_ASSERT(false);
	}
}
