#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cppunit/extensions/HelperMacros.h>

#include "conversion.h"

#include "sz4/block.h"
#include "sz4/path.h"
#include "sz4/load_file_locked.h"

class Sz4BlockTestCase : public CPPUNIT_NS::TestFixture
{
	std::vector<sz4::value_time_pair<unsigned, unsigned> > m_v;
	void searchTest();
	void weigthedSumTest();
	void weigthedSumTest2();
	void pathParseTest();
	void blockLoadTest();

	CPPUNIT_TEST_SUITE( Sz4BlockTestCase );
	CPPUNIT_TEST( searchTest );
	CPPUNIT_TEST( weigthedSumTest );
	CPPUNIT_TEST( weigthedSumTest2 );
	CPPUNIT_TEST( pathParseTest );
	CPPUNIT_TEST( blockLoadTest );
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
};


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4BlockTestCase );

void Sz4BlockTestCase::setUp() {
	m_v.push_back(sz4::make_value_time_pair(1u, 1u));
	m_v.push_back(sz4::make_value_time_pair(3u, 3u));
	m_v.push_back(sz4::make_value_time_pair(5u, 5u));
	m_v.push_back(sz4::make_value_time_pair(7u, 7u));
	m_v.push_back(sz4::make_value_time_pair(9u, 9u));
}

void Sz4BlockTestCase::searchTest() {
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 0u)->value == 1u);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 1u)->value == 1u);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 2u)->value == 3u);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 3u)->value == 3u);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 4u)->value == 5u);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 5u)->value == 5u);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 7u)->value == 7u);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 8u)->value == 9u);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 9u)->value == 9u);
	CPPUNIT_ASSERT(search_entry_for_time(m_v.begin(), m_v.end(), 10u) == m_v.end());
}

void Sz4BlockTestCase::weigthedSumTest() {
	std::pair<typename sz4::value_sum<unsigned>::type, typename sz4::time_difference<unsigned>::type> wsum;
	std::vector<sz4::value_time_pair<unsigned, unsigned> > v = m_v;

	sz4::block<unsigned, unsigned> block(0u);
	block.set_data(v);

	wsum = block.weighted_sum(0u, 1u);
	CPPUNIT_ASSERT_EQUAL(1ull, wsum.first);
	CPPUNIT_ASSERT_EQUAL(1l, wsum.second);

	wsum = block.weighted_sum(0u, 2u);
	CPPUNIT_ASSERT_EQUAL(4ull, wsum.first);
	CPPUNIT_ASSERT_EQUAL(2l, wsum.second);

	wsum = block.weighted_sum(0u, 4u);
	CPPUNIT_ASSERT_EQUAL(12ull, wsum.first);
	CPPUNIT_ASSERT_EQUAL(4l, wsum.second);

	wsum = block.weighted_sum(0u, 100u);
	CPPUNIT_ASSERT_EQUAL(1ull + 2ull * (3ull + 5ull + 7ull + 9ull), wsum.first);
	CPPUNIT_ASSERT_EQUAL(9l, wsum.second);

}

void Sz4BlockTestCase::weigthedSumTest2() {
	std::vector<sz4::value_time_pair<short, unsigned> > v;
	sz4::block<short, unsigned> block(0u);
	std::pair<typename sz4::value_sum<short>::type, typename sz4::time_difference<unsigned>::type> wsum;

	v.push_back(sz4::make_value_time_pair((short)1, 1u));
	v.push_back(sz4::make_value_time_pair(std::numeric_limits<short>::min(), 2u));
	v.push_back(sz4::make_value_time_pair((short)1, 3u));
	block.set_data(v);

	wsum = block.weighted_sum(0u, 3u);
	CPPUNIT_ASSERT_EQUAL(2, wsum.first);
	CPPUNIT_ASSERT_EQUAL(2l, wsum.second);
}

void Sz4BlockTestCase::pathParseTest() {
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(21), sz4::path_to_date<sz4::second_time_t>(L"0000000021.sz4"));
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(1), sz4::path_to_date<sz4::second_time_t>(L"0000000001.sz4"));
	CPPUNIT_ASSERT_EQUAL(sz4::invalid_time_value<sz4::second_time_t>::value(), sz4::path_to_date<sz4::second_time_t>(L"000000001.sz4"));

	sz4::nanosecond_time_t t[] = { {1, 1}, { 2, 2 }, sz4::invalid_time_value<sz4::nanosecond_time_t>::value() };
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

	sz4::block<short, unsigned> block(0u);
	block.set_data(v);

	CPPUNIT_ASSERT_EQUAL(7u, block.end_time());

	std::pair<typename sz4::value_sum<short>::type, typename sz4::time_difference<unsigned>::type> wsum;
	wsum = block.weighted_sum(0u, 7u);
	CPPUNIT_ASSERT_EQUAL(1 + 2 + 3 + 4 + 5 + 7, wsum.first);
	CPPUNIT_ASSERT_EQUAL(6l, wsum.second);

	unlink(SC::S2A(file_name.str()).c_str());
}

