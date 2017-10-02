#include "unit_test_common.h"

class Sz4RPNParam : public CPPUNIT_NS::TestFixture
{
	void test1();
	void test2();

	CPPUNIT_TEST_SUITE( Sz4RPNParam );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST( test2 );
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
};

namespace {

class IPKContainerMock1 : public mocks::IPKContainerMockBase {
	TParam* param;
public:
	IPKContainerMock1() : param(new TParam(NULL, &m_config, L"1", FormulaType::DEFINABLE, ParamType::DEFINABLE)) {
		param->SetDataType(TParam::DOUBLE);
		param->SetName(L"A:B:C1");
		AddParam(param);

		m_config.AddDrawDefinable(param);
	}
	TParam* DoGetParam(const std::wstring&) { return param; }
};

}


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4RPNParam );

void Sz4RPNParam::setUp() {
}

namespace {

struct test_types {
	typedef IPKContainerMock1 ipk_container_type;
	typedef mocks::mock_param_factory param_factory;
};

}



void Sz4RPNParam::test1() {
	IPKContainerMock1 mock;
	sz4::base_templ<test_types> base(L"", &mock);
	auto buff = base.buffer_for_param(mock.GetParam(L""));

	sz4::weighted_sum<double, sz4::second_time_t> sum;
	sz4::weighted_sum<double, sz4::second_time_t>::time_diff_type weight;
	buff->get_weighted_sum(mock.GetParam(L""), 100u, 200u, PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(100., double(sum.sum(weight)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(100), weight);
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());
}

namespace rpn_unit_test {

class IPKContainerMock2 : public mocks::IPKContainerMockBase {
	TParam* param;
	TParam* param2;
public:
	IPKContainerMock2() : param(new TParam(NULL, NULL, std::wstring(), FormulaType::NONE, ParamType::REAL)),
				param2(new TParam(NULL, NULL, L"(A:B:C) 1 +", FormulaType::DEFINABLE, ParamType::DEFINABLE))
	 {
		param->SetDataType(TParam::SHORT);
		param->SetName(L"A:B:C");
		AddParam(param);
		m_config.AddDrawDefinable(param);

		param2->SetDataType(TParam::DOUBLE);
		param2->SetName(L"A:B:D");
		AddParam(param2);
		m_config.AddDrawDefinable(param2);

	}

	TParam* DoGetParam(const std::wstring& name) {
		if (name == L"BASE:A:B:C")
			return param;

		if (name == L"BASE:A:B:D")
			return param2;

		assert(false);
		return NULL;
	}
};

struct test_types {
	typedef IPKContainerMock2 ipk_container_type;
	typedef mocks::mock_param_factory param_factory;
};

}


void Sz4RPNParam::test2() {
	rpn_unit_test::IPKContainerMock2 mock;

	std::wstringstream base_dir_name;
	base_dir_name << L"/tmp/sz4_rpn_param" << getpid() << L"." << time(NULL) << L".tmp";
	boost::filesystem::wpath base_path(base_dir_name.str());
	boost::filesystem::wpath param_dir(base_path / L"BASE/szbase/A/B/C");
	boost::filesystem::create_directories(param_dir);

#if BOOST_FILESYSTEM_VERSION == 3
	sz4::base_templ<rpn_unit_test::test_types> base(base_path.wstring(), &mock);
#else
	sz4::base_templ<rpn_unit_test::test_types> base(base_path.file_string(), &mock);
#endif
	auto buff = base.buffer_for_param(mock.GetParam(L"BASE:A:B:D"));

	
	TestObserver o;
#if BOOST_FILESYSTEM_VERSION == 3
	base.param_monitor().add_observer(&o, mock.GetParam(L"BASE:A:B:C"), param_dir.native(), 10);
#else
	base.param_monitor().add_observer(&o, mock.GetParam(L"BASE:A:B:C"), param_dir.external_file_string(), 10);
#endif
	{
		sz4::weighted_sum<double, sz4::second_time_t> sum;
		buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:D"), 100u, 200u, PT_SEC10, sum);
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.weight());
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(100), sum.no_data_weight());
		CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

		std::wstringstream file_name;
		file_name << std::setfill(L'0') << std::setw(10) << 100 << L".sz4";
		std::string file_path_str;
#if BOOST_FILESYSTEM_VERSION == 3
		file_path_str = (param_dir / file_name.str()).native();
#else
		file_path_str = (param_dir / file_name.str()).external_file_string();
#endif
		int fd = 0;
		CPPUNIT_ASSERT_NO_THROW(fd = sz4::open_writelock(file_path_str.c_str(), O_WRONLY | O_CREAT));

		char buf[3];
		short v = 10;
		unsigned char t = 0x32;

		memcpy(buf, &v, 2);
		memcpy(buf + 2, &t, 1);

		write(fd, buf, sizeof(buf));

		sz4::close_unlock(fd);
	}

	CPPUNIT_ASSERT(o.wait_for(1, 2));
	{
		sz4::weighted_sum<double, sz4::second_time_t> sum;
		sz4::weighted_sum<double, sz4::second_time_t>::time_diff_type weight;
		buff->get_weighted_sum(mock.GetParam(L"BASE:A:B:D"), 100u, 200u, PT_SEC10, sum);
		CPPUNIT_ASSERT_EQUAL(550., double(sum.sum(weight)));
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), weight);
		CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
		CPPUNIT_ASSERT_EQUAL(false, sum.fixed());
	}
}

