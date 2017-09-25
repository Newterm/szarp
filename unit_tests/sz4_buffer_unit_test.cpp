#include "unit_test_common.h"

class Sz4BufferTestCase : public CPPUNIT_NS::TestFixture
{
	void test1();
	void test2();
	void searchTest();

	CPPUNIT_TEST_SUITE( Sz4BufferTestCase );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST( test2 );
	CPPUNIT_TEST( searchTest );
	CPPUNIT_TEST_SUITE_END();

	mocks::IPKContainerMock m_mock;
	sz4::base_templ<mocks::mock_types> m_base;
	boost::filesystem::wpath m_base_dir;
	boost::filesystem::wpath m_param_dir;
public:
	Sz4BufferTestCase() : m_base(L"", &m_mock) {}
	void setUp();
	void tearDown();
};


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4BufferTestCase );

void Sz4BufferTestCase::setUp() {
	m_base_dir = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
	boost::filesystem::create_directories(m_base_dir);

	m_param_dir = m_base_dir / L"A" / L"A" / L"A";
	boost::filesystem::create_directories(m_param_dir);
}

void Sz4BufferTestCase::tearDown() {
	boost::filesystem::remove_all(m_base_dir);
}

void Sz4BufferTestCase::test1() {

	for (int i = 1000; i < 2000; i += 100) {
		std::ostringstream oss;
		unsigned data = 10;
		oss.write((const char*) &data, sizeof(data));
		unsigned char delta = 50;
		oss.write((const char*) &delta, sizeof(delta));
		auto s = oss.str();

		std::wstringstream name;
		name << std::setfill(L'0') << std::setw(10) << i << L".sz4";
		save_bz2_file({ s.begin(), s.end() }, m_param_dir / name.str());
	}


	TParam param(NULL);
	param.SetParamId(0);
	param.SetName(L"A:A:A");
	param.SetTimeType(TParam::SECOND);
	param.SetDataType(TParam::INT);

	SzbParamMonitor monitor(L"");
	mocks::IPKContainerMock container_mock;
	container_mock.add_param(&param);
	auto buffer = new sz4::buffer_templ<sz4::base_templ<mocks::mock_types>>(&m_base, &monitor, &container_mock, L"TEST", m_base_dir.wstring());
	sz4::weighted_sum<int, sz4::second_time_t> sum;
	sz4::weighted_sum<int, sz4::second_time_t>::time_diff_type weight;

	buffer->get_weighted_sum(&param, sz4::second_time_t(1000), sz4::second_time_t(2000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(5000), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(500), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(0), sz4::second_time_t(1000), PT_SEC10, sum);
	sum.sum(weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(1000), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(2000), sz4::second_time_t(2050), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1050), sz4::second_time_t(1100), PT_SEC10, sum);
	sum.sum(weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1050), sz4::second_time_t(1150), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1025), sz4::second_time_t(1175), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(30 * 25), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(75), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(75), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1025), sz4::second_time_t(1025), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(0), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer->get_weighted_sum(&param, sz4::second_time_t(1075), sz4::second_time_t(1075), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(0), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	delete buffer;
}

void Sz4BufferTestCase::test2() {
	std::ostringstream oss;

	for (int i = 1000; i < 2000; i += 100) {
		oss.str("");

		unsigned data = 10;
		oss.write((const char*) &data, sizeof(data));
		unsigned char delta = 50;
		oss.write((const char*) &delta, sizeof(delta));
		auto s = oss.str();

		std::wstringstream name;
		name << std::setfill(L'0') << std::setw(10) << i << L".sz4";
		save_bz2_file({ s.begin(), s.end() }, m_param_dir / name.str());
	}


	TParam param(NULL);
	param.SetName(L"A:A:A");
	param.SetTimeType(TParam::SECOND);
	param.SetDataType(TParam::INT);

	SzbParamMonitor monitor(L"");
	mocks::IPKContainerMock container_mock;
	container_mock.add_param(&param);
	sz4::buffer_templ<sz4::base_templ<mocks::mock_types>> buffer(&m_base, &monitor, &container_mock, L"TEST", m_base_dir.wstring());
	sz4::weighted_sum<int, sz4::second_time_t> sum;
	sz4::weighted_sum<int, sz4::second_time_t>::time_diff_type weight;

	buffer.get_weighted_sum(&param, sz4::second_time_t(1900), sz4::second_time_t(2000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(50), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

	TestObserver o;
	monitor.add_observer(&o, &param, SC::S2A(m_param_dir.wstring()), 1);
	{
		unsigned data = 20;
		oss.write((const char*) &data, sizeof(data));
		unsigned char delta = 50;
		oss.write((const char*) &delta, sizeof(delta));
		auto s = oss.str();

		std::wstringstream name;
		name << std::setfill(L'0') << std::setw(10) << 1900 << L".sz4";
		save_bz2_file({ s.begin(), s.end() }, m_param_dir / name.str());
	}
	o.wait_for(1);

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer.get_weighted_sum(&param, sz4::second_time_t(1900), sz4::second_time_t(2000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(1500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(100), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	{
		unsigned data = 30;
		oss.write((const char*) &data, sizeof(data));
		//3000;
		unsigned char delta[] = { 0x83u, 0xe8u };
		oss.write((const char*) delta, sizeof(delta));
		auto s = oss.str();

		std::wstringstream name;
		name << std::setfill(L'0') << std::setw(10) << 1900 << L".sz4";
		save_bz2_file({ s.begin(), s.end() }, m_param_dir / name.str());
	}
	o.wait_for(2);

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer.get_weighted_sum(&param, sz4::second_time_t(1900), sz4::second_time_t(3000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sz4::value_sum<int>::type(31500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(1100), weight);
	CPPUNIT_ASSERT_EQUAL(sz4::time_difference<sz4::second_time_t>::type(0), sum.no_data_weight());
	CPPUNIT_ASSERT_EQUAL(true, sum.fixed());

	sum = sz4::weighted_sum<int, sz4::second_time_t>();
	buffer.get_weighted_sum(&param, sz4::second_time_t(1900), sz4::second_time_t(4000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(false, sum.fixed());

}

void Sz4BufferTestCase::searchTest() {

	for (int i = 0; i < 2100; i += 100) {
		std::ostringstream oss;

		int j;
		for (j = 0; j < 100 - (2000 - i) / 100 - 10; j++) {
			unsigned data = j % 25;
			oss.write((const char*) &data, sizeof(data));
			unsigned char delta = 1;
			oss.write((const char*) &delta, sizeof(delta));
		}

		unsigned data = i / 100 + 10000;
		oss.write((const char*) &data, sizeof(data));
		unsigned char delta = 1;
		oss.write((const char*) &delta, sizeof(delta));
		auto s = oss.str();

		std::wstringstream name;
		name << std::setfill(L'0') << std::setw(10) << i << L".sz4";
		save_bz2_file({ s.begin(), s.end() }, m_param_dir / name.str());
	}


	TParam param(NULL);
	param.SetName(L"A:A:A");
	param.SetTimeType(TParam::SECOND);
	param.SetDataType(TParam::INT);

	
	SzbParamMonitor monitor(L"");
	mocks::IPKContainerMock container_mock;
	container_mock.add_param(&param);
	sz4::buffer_templ<sz4::base_templ<mocks::mock_types>> buffer(&m_base, &monitor, &container_mock, L"TEST", m_base_dir.wstring());

	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(0), buffer.search_data_right(&param, sz4::second_time_t(0), sz4::second_time_t(2000), PT_SEC10, test_search_condition(0)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(100), buffer.search_data_right(&param, sz4::second_time_t(100), sz4::second_time_t(2000), PT_SEC10, test_search_condition(0)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(125), buffer.search_data_right(&param, sz4::second_time_t(101), sz4::second_time_t(2000), PT_SEC10, test_search_condition(0)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(1), buffer.search_data_right(&param, sz4::second_time_t(0), sz4::second_time_t(2000), PT_SEC10, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(101), buffer.search_data_right(&param, sz4::second_time_t(100), sz4::second_time_t(2000), PT_SEC10, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(224), buffer.search_data_right(&param, sz4::second_time_t(190), sz4::second_time_t(2000), PT_SEC10, test_search_condition(24)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(224), buffer.search_data_right(&param, sz4::second_time_t(190), sz4::second_time_t(2000), PT_SEC10, test_search_condition(24)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, buffer.search_data_right(&param, sz4::second_time_t(10000), sz4::second_time_t(20000), PT_SEC10, test_search_condition(181)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, buffer.search_data_right(&param, sz4::second_time_t(1885), sz4::second_time_t(1900), PT_SEC10, test_search_condition(0)));
	for (int i = 0; i < 20; i++) {
		CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(100 * i + 70 + i), buffer.search_data_right(&param, sz4::second_time_t(0), sz4::second_time_t(2000), PT_SEC10, test_search_condition(10000 + i)));
	}
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, buffer.search_data_right(&param, sz4::second_time_t(182), sz4::second_time_t(2000), PT_SEC10, test_search_condition(181)));

	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(2000), buffer.search_data_left(&param, sz4::second_time_t(2000), sz4::second_time_t(0), PT_SEC10, test_search_condition(0)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(150), buffer.search_data_left(&param, sz4::second_time_t(198), sz4::second_time_t(100), PT_SEC10, test_search_condition(0)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, buffer.search_data_left(&param, sz4::second_time_t(1999), sz4::second_time_t(1998), PT_SEC10, test_search_condition(0)));
	for (int i = 0; i < 20; i++) {
		CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(100 * i + 70 + i), buffer.search_data_left(&param, sz4::second_time_t(2000), sz4::second_time_t(0), PT_SEC10, test_search_condition(10000 + i)));
	}

}
