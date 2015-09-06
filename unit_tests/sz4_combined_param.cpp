#include "unit_test_common.h"

class Sz4CombinedParam : public CPPUNIT_NS::TestFixture
{
	void test1();

	CPPUNIT_TEST_SUITE( Sz4CombinedParam );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST_SUITE_END();
public:
};

namespace {

class IPKContainerMock : public mocks::IPKContainerMockBase {
	TSzarpConfig config;
	TParam *param;
	TParam *param2;
	TParam *param3;
public:
	IPKContainerMock() : param(new TParam(NULL, NULL, L"1", TParam::DEFINABLE, TParam::P_DEFINABLE))
				, param2(new TParam(NULL, NULL, L"2", TParam::DEFINABLE, TParam::P_DEFINABLE))
				, param3(new TParam(NULL, NULL, L"(*:*:msw) (*:*:lsw) :", TParam::DEFINABLE, TParam::P_DEFINABLE)) {

		param->SetName(L"A:B:msw");
		AddParam(param);

		param2->SetName(L"A:B:lsw");
		AddParam(param2);

		param3->SetName(L"A:B:C");
		AddParam(param3);

		m_config.AddDrawDefinable(param);
		m_config.AddDrawDefinable(param2);
		m_config.AddDrawDefinable(param3);
	}

	TParam* DoGetParam(const std::wstring& s) {
		if (s.find(L"lsw") != std::string::npos)
			return param;
		else if (s.find(L"msw") != std::string::npos)
			return param2;
		else
			return param3;
	}
};

struct test_types {
	typedef IPKContainerMock ipk_container_type;
	typedef mocks::mock_param_factory param_factory;
};

}

void Sz4CombinedParam::test1() {
	IPKContainerMock mock;

	boost::filesystem::wpath tmp_path = boost::filesystem::unique_path();
	sz4::base_templ<test_types> base(tmp_path.wstring(), &mock);

	sz4::weighted_sum<int, sz4::second_time_t> sum;
	sz4::weighted_sum<int, sz4::second_time_t>::time_diff_type weight;
	base.get_weighted_sum(mock.GetParam(L""), 0u, 1u, PT_SEC, sum);
	CPPUNIT_ASSERT_EQUAL(65538, int(sum.sum(weight)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(1), weight);

	boost::filesystem::remove_all(tmp_path);
}

CPPUNIT_TEST_SUITE_REGISTRATION( Sz4CombinedParam );
