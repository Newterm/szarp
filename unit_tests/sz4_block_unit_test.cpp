#include <vector>
#include <iostream>
#include <cppunit/extensions/HelperMacros.h>
#include "sz4/block.h"


class Sz4BlockTestCase : public CPPUNIT_NS::TestFixture
{
	void searchTest();
public:
	CPPUNIT_TEST_SUITE( Sz4BlockTestCase );
	CPPUNIT_TEST( searchTest );
	CPPUNIT_TEST_SUITE_END();
};


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4BlockTestCase );

void Sz4BlockTestCase::searchTest() {
	std::vector<sz4::value_time_pair<unsigned, unsigned> > v;

	v.push_back(sz4::make_value_time_pair(1u, 1u));
	v.push_back(sz4::make_value_time_pair(3u, 3u));
	v.push_back(sz4::make_value_time_pair(5u, 5u));
	v.push_back(sz4::make_value_time_pair(7u, 7u));
	v.push_back(sz4::make_value_time_pair(9u, 9u));

	CPPUNIT_ASSERT(search_entry_for_time(v.begin(), v.end(), 0u)->value == 1u);
	CPPUNIT_ASSERT(search_entry_for_time(v.begin(), v.end(), 1u)->value == 1u);
	CPPUNIT_ASSERT(search_entry_for_time(v.begin(), v.end(), 2u)->value == 3u);
	CPPUNIT_ASSERT(search_entry_for_time(v.begin(), v.end(), 3u)->value == 3u);
	CPPUNIT_ASSERT(search_entry_for_time(v.begin(), v.end(), 4u)->value == 5u);
	CPPUNIT_ASSERT(search_entry_for_time(v.begin(), v.end(), 5u)->value == 5u);
	CPPUNIT_ASSERT(search_entry_for_time(v.begin(), v.end(), 7u)->value == 7u);
	CPPUNIT_ASSERT(search_entry_for_time(v.begin(), v.end(), 8u)->value == 9u);
	CPPUNIT_ASSERT(search_entry_for_time(v.begin(), v.end(), 9u)->value == 9u);
	CPPUNIT_ASSERT(search_entry_for_time(v.begin(), v.end(), 10u) == v.end());
}
