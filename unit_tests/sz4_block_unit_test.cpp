#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <boost/filesystem/path.hpp>
#include <cppunit/extensions/HelperMacros.h>

#include "conversion.h"

#include "sz4/defs.h"
#include "sz4/time.h"
#include "sz4/block.h"
#include "sz4/path.h"
#include "sz4/load_file_locked.h"

#include "test_serach_condition.h"

class Sz4BlockTestCase : public CPPUNIT_NS::TestFixture
{
	std::vector<sz4::value_time_pair<int, sz4::second_time_t> > m_v;
	void searchTest();
	void weigthedSumTest();
	void weigthedSumTest2();
	void pathParseTest();
	void blockLoadTest();
	void searchDataTest();
	void testBigNum();

	CPPUNIT_TEST_SUITE( Sz4BlockTestCase );
	CPPUNIT_TEST( searchTest );
	CPPUNIT_TEST( weigthedSumTest );
	CPPUNIT_TEST( weigthedSumTest2 );
	CPPUNIT_TEST( pathParseTest );
	CPPUNIT_TEST( blockLoadTest );
	CPPUNIT_TEST( searchDataTest );
	CPPUNIT_TEST( testBigNum );
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
};


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4BlockTestCase );

void Sz4BlockTestCase::setUp() {
	typedef sz4::value_time_pair<int, sz4::second_time_t> pair_type;
	m_v.push_back(sz4::make_value_time_pair<pair_type>(1, 1u));
	m_v.push_back(sz4::make_value_time_pair<pair_type>(3, 3u));
	m_v.push_back(sz4::make_value_time_pair<pair_type>(5, 5u));
	m_v.push_back(sz4::make_value_time_pair<pair_type>(7, 7u));
	m_v.push_back(sz4::make_value_time_pair<pair_type>(9, 9u));
}

void Sz4BlockTestCase::searchTest() {
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 0u)->value == 1);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 1u)->value == 3);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 2u)->value == 3);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 3u)->value == 5);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 4u)->value == 5);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 5u)->value == 7);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 7u)->value == 9);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 8u)->value == 9);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 9u) == m_v.end());
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 10u) == m_v.end());
}

void Sz4BlockTestCase::weigthedSumTest() {
	sz4::weighted_sum<int, sz4::second_time_t> wsum;
	std::vector<sz4::value_time_pair<int, sz4::second_time_t> > v = m_v;

	sz4::concrete_block<int, sz4::second_time_t> block(0u);
	block.set_data(v);

	block.get_weighted_sum(0u, sz4::second_time_t(1), wsum);
	CPPUNIT_ASSERT_EQUAL(1ll, wsum.sum());
	CPPUNIT_ASSERT_EQUAL(1l, wsum.weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(0u, 2u, wsum);
	CPPUNIT_ASSERT_EQUAL(4ll, wsum.sum());
	CPPUNIT_ASSERT_EQUAL(2l, wsum.weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(0u, 4u, wsum);
	CPPUNIT_ASSERT_EQUAL(12ll, wsum.sum());
	CPPUNIT_ASSERT_EQUAL(4l, wsum.weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(0u, 100u, wsum);
	CPPUNIT_ASSERT_EQUAL(1ll + 2ll * (3ll + 5ll + 7ll + 9ll), wsum.sum());
	CPPUNIT_ASSERT_EQUAL(9l, wsum.weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(2u, 3u, wsum);
	CPPUNIT_ASSERT_EQUAL(3ll, wsum.sum());
	CPPUNIT_ASSERT_EQUAL(1l, wsum.weight());
	CPPUNIT_ASSERT_EQUAL(0l, wsum.no_data_weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(1u, 3u, wsum);
	CPPUNIT_ASSERT_EQUAL(6ll, wsum.sum());
	CPPUNIT_ASSERT_EQUAL(2l, wsum.weight());
	CPPUNIT_ASSERT_EQUAL(0l, wsum.no_data_weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(1u, 1u, wsum);
	CPPUNIT_ASSERT_EQUAL(0ll, wsum.sum());
	CPPUNIT_ASSERT_EQUAL(0l, wsum.weight());
	CPPUNIT_ASSERT_EQUAL(0l, wsum.no_data_weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(2u, 2u, wsum);
	CPPUNIT_ASSERT_EQUAL(0ll, wsum.sum());
	CPPUNIT_ASSERT_EQUAL(0l, wsum.weight());
	CPPUNIT_ASSERT_EQUAL(0l, wsum.no_data_weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(0u, 0u, wsum);
	CPPUNIT_ASSERT_EQUAL(0ll, wsum.sum());
	CPPUNIT_ASSERT_EQUAL(0l, wsum.weight());
	CPPUNIT_ASSERT_EQUAL(0l, wsum.no_data_weight());
}

void Sz4BlockTestCase::weigthedSumTest2() {
	typedef sz4::value_time_pair<short, sz4::second_time_t> pair_type;
	std::vector<pair_type> v;
	sz4::concrete_block<short, unsigned> block(0u);
	sz4::weighted_sum<short, sz4::second_time_t> wsum;

	v.push_back(sz4::make_value_time_pair<pair_type>((short)1, 1u));
	v.push_back(sz4::make_value_time_pair<pair_type>(std::numeric_limits<short>::min(), 2u));
	v.push_back(sz4::make_value_time_pair<pair_type>((short)1, 3u));
	block.set_data(v);

	block.get_weighted_sum(0u, 3u, wsum);
	CPPUNIT_ASSERT_EQUAL(2ll, wsum.sum());
	CPPUNIT_ASSERT_EQUAL(2l, wsum.weight());
}

void Sz4BlockTestCase::pathParseTest() {
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(21), sz4::path_to_date<sz4::second_time_t>(L"0000000021.sz4"));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(1), sz4::path_to_date<sz4::second_time_t>(L"0000000001.sz4"));
	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, sz4::path_to_date<sz4::second_time_t>(L"000000001.sz4"));

	sz4::nanosecond_time_t t[] = { sz4::nanosecond_time_t(1, 1), sz4::nanosecond_time_t(2, 2 ), sz4::invalid_time_value<sz4::nanosecond_time_t>::value };
	CPPUNIT_ASSERT(t[0] == sz4::path_to_date<sz4::nanosecond_time_t>(L"00000000010000000001.sz4"));
	CPPUNIT_ASSERT(t[1] == sz4::path_to_date<sz4::nanosecond_time_t>(L"00000000020000000002.sz4"));
	CPPUNIT_ASSERT(t[2] == sz4::path_to_date<sz4::nanosecond_time_t>(L"00000000020000.sz4"));
}

void Sz4BlockTestCase::blockLoadTest() {
	char file_content[] = {
		 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
		 0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
		 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
		 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
		 0x05, 0x00, 0x05, 0x00, 0x00, 0x00,
		 0x00, 0x80, 0x06, 0x00, 0x00, 0x00,
		 0x07, 0x00, 0x07, 0x00, 0x00, 0x00,
		};

	std::wstringstream file_name;
	file_name << L"/tmp/sz4block_unit_test." << getpid() << "." << time(NULL) << L".tmp";

	{
		std::ofstream ofs(SC::S2A(file_name.str()).c_str(), std::ios_base::binary);
		ofs.write(file_content, sizeof(file_content));
	}

	std::vector<sz4::value_time_pair<short, unsigned> > v;
	v.resize(7);
	CPPUNIT_ASSERT(sz4::load_file_locked(file_name.str(), reinterpret_cast<char*>(&v[0]), sizeof(file_content)));
	for (size_t i = 0; i < 7; i++) {
		if (i != 5)
			CPPUNIT_ASSERT(v.at(i).value == short(i + 1));
		else
			CPPUNIT_ASSERT(v.at(i).value == std::numeric_limits<short>::min());
	}

	sz4::concrete_block<short, unsigned> block(0u);
	block.set_data(v);

	CPPUNIT_ASSERT_EQUAL(7u, block.end_time());

	sz4::weighted_sum<short, unsigned> wsum;
	block.get_weighted_sum(0u, 7u, wsum);
	CPPUNIT_ASSERT_EQUAL(1ll + 2 + 3 + 4 + 5 + 7, wsum.sum());
	CPPUNIT_ASSERT_EQUAL(6l, wsum.weight());

	unlink(SC::S2A(file_name.str()).c_str());
}

void Sz4BlockTestCase::testBigNum() {
	sz4::concrete_block<short, sz4::second_time_t> block(10000000u);

	typedef sz4::value_time_pair<short, sz4::second_time_t> pair_type;
	std::vector<pair_type> v;
	v.push_back(sz4::make_value_time_pair<pair_type>(short(1500), 10000000u + 2 * 3600 * 24 * 31u)); 
	block.set_data(v);

	sz4::weighted_sum<short, unsigned> wsum;
	block.get_weighted_sum(10000000u, 10000000u + 1 * 3600 * 24 * 31, wsum);
	CPPUNIT_ASSERT_EQUAL(4017600000ll, wsum.sum());
}

void Sz4BlockTestCase::searchDataTest() {
	typedef sz4::value_time_pair<int, sz4::second_time_t> pair_type;
	sz4::concrete_block<int, sz4::second_time_t> block(0u);
	std::vector<pair_type> v = m_v;

	v.push_back(sz4::make_value_time_pair<pair_type>(3, 11u));
	block.set_data(v);

	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(0), block.search_data_right(0u, 10u, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(0), block.search_data_right(0u, 1u, test_search_condition(1)));

	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, block.search_data_right(1u, 10u, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, block.search_data_right(2u, 10u, test_search_condition(1)));

	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(1), block.search_data_right(0u, 10u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(2), block.search_data_right(2u, 10u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(9), block.search_data_right(3u, 10u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, block.search_data_right(3u, 9u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(9), block.search_data_right(9u, 11u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(10), block.search_data_right(10u, 11u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, block.search_data_right(10u, 11u, test_search_condition(5)));
	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, block.search_data_right(11u, 12u, test_search_condition(3)));

	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, block.search_data_right(2u, 11u, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(7), block.search_data_right(0u, 10u, test_search_condition(9)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(7), block.search_data_right(4u, 10u, test_search_condition(9)));


	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(0), block.search_data_left(10u, 0u, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(0), block.search_data_left(1u, 0u, test_search_condition(1)));

	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, block.search_data_left(10u, 1u, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, block.search_data_left(100u, 11u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(4), block.search_data_left(100u, 0, test_search_condition(5)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(10), block.search_data_left(10u, 0, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, block.search_data_left(8u, 3u, test_search_condition(3)));

	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(10), block.search_data_left(11u, 10u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value, block.search_data_left(11u, 10u, test_search_condition(5)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(2), block.search_data_left(8, 0u, test_search_condition(3)));

}

