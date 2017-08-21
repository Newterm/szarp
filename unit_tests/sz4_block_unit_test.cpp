#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <cppunit/extensions/HelperMacros.h>

#include "conversion.h"

#include "unit_test_common.h"

class Sz4BlockTestCase : public CPPUNIT_NS::TestFixture
{
	std::vector<sz4::value_time_pair<int, sz4::second_time_t> > m_v;
	sz4::block_cache m_cache;
	void searchTest();
	void weigthedSumTest();
	void weigthedSumTest2();
	void pathTest();
	void blockLoadTest();
	void searchDataTest();
	void testBigNum();

	CPPUNIT_TEST_SUITE( Sz4BlockTestCase );
	CPPUNIT_TEST( searchTest );
	CPPUNIT_TEST( weigthedSumTest );
	CPPUNIT_TEST( weigthedSumTest2 );
	CPPUNIT_TEST( pathTest );
	CPPUNIT_TEST( blockLoadTest );
	CPPUNIT_TEST( searchDataTest );
	CPPUNIT_TEST( testBigNum );
	CPPUNIT_TEST_SUITE_END();

	boost::filesystem::wpath m_temp_dir;
public:
	void setUp() override;
	void tearDown() override;
};


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4BlockTestCase );

void Sz4BlockTestCase::setUp() {
	typedef sz4::value_time_pair<int, sz4::second_time_t> pair_type;
	m_v.push_back(sz4::make_value_time_pair<pair_type>(1, 1u));
	m_v.push_back(sz4::make_value_time_pair<pair_type>(3, 3u));
	m_v.push_back(sz4::make_value_time_pair<pair_type>(5, 5u));
	m_v.push_back(sz4::make_value_time_pair<pair_type>(7, 7u));
	m_v.push_back(sz4::make_value_time_pair<pair_type>(9, 9u));

	m_temp_dir = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
	boost::filesystem::create_directories(m_temp_dir);
}

void Sz4BlockTestCase::tearDown() {
	boost::filesystem::remove_all(m_temp_dir);
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

	sz4::concrete_block<int, sz4::second_time_t> block(0u, &m_cache);
	block.set_data(v);

	sz4::weighted_sum<int, sz4::second_time_t>::time_diff_type weight;
	block.get_weighted_sum(0u, sz4::second_time_t(1), wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(1), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(1l, weight);

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(0u, 2u, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(4), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(2l, weight);

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(0u, 4u, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(12), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(4l, weight);

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(0u, 100u, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(1 + 2 * (3 + 5 + 7 + 9)), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(9l, weight);

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(2u, 3u, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(3), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(1l, weight);
	CPPUNIT_ASSERT_EQUAL(0l, wsum.no_data_weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(1u, 3u, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(6), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(2l, weight);
	CPPUNIT_ASSERT_EQUAL(0l, wsum.no_data_weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(1u, 1u, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(0), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(0l, weight);
	CPPUNIT_ASSERT_EQUAL(0l, wsum.no_data_weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(2u, 2u, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(0), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(0l, weight);
	CPPUNIT_ASSERT_EQUAL(0l, wsum.no_data_weight());

	wsum = sz4::weighted_sum<int, sz4::second_time_t>();
	block.get_weighted_sum(0u, 0u, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(0), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(0l, weight);
	CPPUNIT_ASSERT_EQUAL(0l, wsum.no_data_weight());
}

void Sz4BlockTestCase::weigthedSumTest2() {
	typedef sz4::value_time_pair<short, sz4::second_time_t> pair_type;
	std::vector<pair_type> v;
	sz4::concrete_block<short, unsigned> block(0u, &m_cache);
	sz4::weighted_sum<short, sz4::second_time_t> wsum;
	sz4::weighted_sum<short, sz4::second_time_t>::time_diff_type weight;

	v.push_back(sz4::make_value_time_pair<pair_type>((short)1, 1u));
	v.push_back(sz4::make_value_time_pair<pair_type>(std::numeric_limits<short>::min(), 2u));
	v.push_back(sz4::make_value_time_pair<pair_type>((short)1, 3u));
	block.set_data(v);

	block.get_weighted_sum(0u, 3u, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(2), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(2l, weight);
}

void Sz4BlockTestCase::pathTest() {
	bool sz4;
	sz4::second_time_t st1[] = { 21, 1, sz4::time_trait<sz4::second_time_t>::invalid_value };
	std::wstring wst1[] = { L"0000000021.sz4", L"0000000001.sz4", L"000000001.sz4" };
	for (int i = 0; i < 3; i++)
		CPPUNIT_ASSERT_EQUAL(st1[i], sz4::path_to_date<sz4::second_time_t>(wst1[i], sz4));

	sz4::nanosecond_time_t st2[] = { {1, 1}, {2, 2}, sz4::time_trait<sz4::nanosecond_time_t>::invalid_value };
	std::wstring wst2[] = { L"00000000010000000001.sz4", L"00000000020000000002.sz4", L"00000000020000.sz4" };
	for (int i = 0; i < 3; i++)
		CPPUNIT_ASSERT_EQUAL(st2[i], sz4::path_to_date<sz4::nanosecond_time_t>(wst2[i], sz4));

	for (int i = 0; i < 2; i++)
		CPPUNIT_ASSERT(wst1[i] == sz4::date_to_path<wchar_t>(st1[i]));

	for (int i = 0; i < 2; i++)
		CPPUNIT_ASSERT(wst2[i] == sz4::date_to_path<wchar_t>(st2[i]));
}

void Sz4BlockTestCase::blockLoadTest() {
	unsigned char file_content[] = {
		 0x01, 0x00, 0x01,
		 0x02, 0x00, 0x01,
		 0x03, 0x00, 0x01,
		 0x04, 0x00, 0x01,
		 0x05, 0x00, 0x01,
		 0x00, 0x80, 0x01,
		 0x07, 0x00, 0x01,
		};

	auto path = m_temp_dir / boost::filesystem::unique_path();
	save_bz2_file({ file_content, file_content + sizeof(file_content) }, path);

	std::vector<unsigned char> bytes;
	CPPUNIT_ASSERT(sz4::load_file_locked(path, bytes));
	auto v = sz4::decode_file<short, unsigned>(&bytes[0], bytes.size(), 0);
	for (size_t i = 0; i < 7; i++) {
		if (i != 5)
			CPPUNIT_ASSERT(v.at(i).value == short(i + 1));
		else
			CPPUNIT_ASSERT(v.at(i).value == std::numeric_limits<short>::min());
	}

	sz4::concrete_block<short, unsigned> block(0u, &m_cache);
	block.set_data(v);

	CPPUNIT_ASSERT_EQUAL(7u, block.end_time());

	sz4::weighted_sum<short, unsigned> wsum;
	sz4::weighted_sum<short, unsigned>::time_diff_type weight;
	block.get_weighted_sum(0u, 7u, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(1 + 2 + 3 + 4 + 5 + 7), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(6l, weight);
}

void Sz4BlockTestCase::testBigNum() {
	sz4::concrete_block<short, sz4::second_time_t> block(10000000u, &m_cache);

	typedef sz4::value_time_pair<short, sz4::second_time_t> pair_type;
	std::vector<pair_type> v;
	v.push_back(sz4::make_value_time_pair<pair_type>(short(1500), 10000000u + 2 * 3600 * 24 * 31u)); 
	block.set_data(v);

	sz4::weighted_sum<short, unsigned> wsum;
	sz4::weighted_sum<short, unsigned>::time_diff_type weight;
	block.get_weighted_sum(10000000u, 10000000u + 1 * 3600 * 24 * 31, wsum);
	CPPUNIT_ASSERT_EQUAL(decltype(wsum)::sum_type(4017600000), wsum.sum(weight));
	CPPUNIT_ASSERT_EQUAL((sz4::weighted_sum<short, unsigned>::time_diff_type(2678400)), weight);
}

void Sz4BlockTestCase::searchDataTest() {
	typedef sz4::value_time_pair<int, sz4::second_time_t> pair_type;
	sz4::concrete_block<int, sz4::second_time_t> block(0u, &m_cache);
	std::vector<pair_type> v = m_v;

	v.push_back(sz4::make_value_time_pair<pair_type>(3, 11u));
	block.set_data(v);

	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(0), block.search_data_right(0u, 10u, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(0), block.search_data_right(0u, 1u, test_search_condition(1)));

	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, block.search_data_right(1u, 10u, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, block.search_data_right(2u, 10u, test_search_condition(1)));

	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(1), block.search_data_right(0u, 10u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(2), block.search_data_right(2u, 10u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(9), block.search_data_right(3u, 10u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, block.search_data_right(3u, 9u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(9), block.search_data_right(9u, 11u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(10), block.search_data_right(10u, 11u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, block.search_data_right(10u, 11u, test_search_condition(5)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, block.search_data_right(11u, 12u, test_search_condition(3)));

	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, block.search_data_right(2u, 11u, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(7), block.search_data_right(0u, 10u, test_search_condition(9)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(7), block.search_data_right(4u, 10u, test_search_condition(9)));


	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(0), block.search_data_left(10u, 0u, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(0), block.search_data_left(1u, 0u, test_search_condition(1)));

	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, block.search_data_left(10u, 1u, test_search_condition(1)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, block.search_data_left(100u, 11u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(4), block.search_data_left(100u, 0, test_search_condition(5)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(10), block.search_data_left(10u, 0, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, block.search_data_left(8u, 3u, test_search_condition(3)));

	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(10), block.search_data_left(11u, 10u, test_search_condition(3)));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::second_time_t>::invalid_value, block.search_data_left(11u, 10u, test_search_condition(5)));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(2), block.search_data_left(8, 0u, test_search_condition(3)));

}

