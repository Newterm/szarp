#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <limits>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cppunit/extensions/HelperMacros.h>
#include <boost/filesystem.hpp>

#include "conversion.h"

#include "szarp_base_common/defines.h"
#include "sz4/defs.h"
#include "sz4/time.h"
#include "sz4/path.h"
#include "sz4/block.h"
#include "sz4/block_cache.h"
#include "sz4/definable_param_cache.h"

#include "test_serach_condition.h"

class Sz4DefinableParamCache : public CPPUNIT_NS::TestFixture
{
	void test1();
	void test2();

	CPPUNIT_TEST_SUITE( Sz4DefinableParamCache );
	CPPUNIT_TEST( test1 );
	CPPUNIT_TEST( test2 );
	CPPUNIT_TEST_SUITE_END();

	sz4::block_cache* m_cache;
public:
	void setUp();
	void tearDown();
};


class definable_param_cache : public sz4::definable_param_cache<int, sz4::nanosecond_time_t> {
public:
	definable_param_cache(SZARP_PROBE_TYPE probe_type, sz4::block_cache* cache) : sz4::definable_param_cache<int, sz4::nanosecond_time_t>(probe_type, cache) {}
	map_type& blocks() { return m_blocks; }
};


CPPUNIT_TEST_SUITE_REGISTRATION( Sz4DefinableParamCache );

void Sz4DefinableParamCache::setUp() {
	m_cache = new sz4::block_cache();
}

void Sz4DefinableParamCache::tearDown() {
	delete m_cache;
}

void Sz4DefinableParamCache::test1() {
	definable_param_cache cache(PT_SEC10, m_cache);
	test_search_condition cond(10);
	int v;
	bool fixed;

	CPPUNIT_ASSERT(!cache.get_value(sz4::make_nanosecond_time(1, 0), v, fixed));

	std::pair<bool, sz4::nanosecond_time_t> r;
	r = cache.search_data_left(sz4::make_nanosecond_time(100, 0), 
					sz4::make_nanosecond_time(1, 0), 
					test_search_condition(1));
	CPPUNIT_ASSERT_EQUAL(false, r.first);
	CPPUNIT_ASSERT_EQUAL(sz4::make_nanosecond_time(100, 0), r.second);

	sz4::nanosecond_time_t t = sz4::make_nanosecond_time(100, 0);
	cache.store_value(10, t, false);
	CPPUNIT_ASSERT(cache.get_value(t, v, fixed));
	CPPUNIT_ASSERT_EQUAL(v, 10);

	r = cache.search_data_left(sz4::make_nanosecond_time(100, 0), 
					sz4::make_nanosecond_time(1, 0), 
					test_search_condition(10));
	CPPUNIT_ASSERT_EQUAL(r.first, true);
	CPPUNIT_ASSERT_EQUAL(sz4::make_nanosecond_time(100, 0), r.second);

	sz4::nanosecond_time_t t1 = sz4::make_nanosecond_time(110, 0);
	cache.store_value(20, t1, true);

	CPPUNIT_ASSERT(cache.get_value(t1, v, fixed));
	CPPUNIT_ASSERT_EQUAL(v, 20);

	r = cache.search_data_left(sz4::make_nanosecond_time(110, 0), 
					sz4::make_nanosecond_time(1, 0), 
					test_search_condition(10));
	CPPUNIT_ASSERT_EQUAL(r.first, true);
	CPPUNIT_ASSERT_EQUAL(sz4::make_nanosecond_time(100, 0), r.second);

	r = cache.search_data_right(sz4::make_nanosecond_time(100, 0), 
					sz4::make_nanosecond_time(200, 0), 
					test_search_condition(10));
	CPPUNIT_ASSERT_EQUAL(r.first, true);
	CPPUNIT_ASSERT_EQUAL(sz4::make_nanosecond_time(100, 0), r.second);


	r = cache.search_data_right(sz4::make_nanosecond_time(100, 0), 
					sz4::make_nanosecond_time(200, 0), 
					test_search_condition(20));
	CPPUNIT_ASSERT_EQUAL(r.first, true);
	CPPUNIT_ASSERT_EQUAL(sz4::make_nanosecond_time(110, 0), r.second);

	sz4::nanosecond_time_t t2 = sz4::make_nanosecond_time(130, 0);
	cache.store_value(30, t2, true);

	r = cache.search_data_right(sz4::make_nanosecond_time(130, 0), 
					sz4::make_nanosecond_time(200, 0), 
					test_search_condition(30));
	CPPUNIT_ASSERT_EQUAL(r.first, true);
	CPPUNIT_ASSERT_EQUAL(sz4::make_nanosecond_time(130, 0), r.second);

	r = cache.search_data_right(sz4::make_nanosecond_time(100, 0), 
					sz4::make_nanosecond_time(200, 0), 
					test_search_condition(30));
	CPPUNIT_ASSERT_EQUAL(r.first, false);
	CPPUNIT_ASSERT_EQUAL(sz4::make_nanosecond_time(120, 0), r.second);

	r = cache.search_data_right(sz4::make_nanosecond_time(100, 0), 
					sz4::make_nanosecond_time(120, 0), 
					test_search_condition(30));
	CPPUNIT_ASSERT_EQUAL(r.first, true);
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::nanosecond_time_t>::is_valid(r.second), false);

	r = cache.search_data_right(sz4::make_nanosecond_time(90, 0), 
					sz4::make_nanosecond_time(100, 0), 
					test_search_condition(10));
	CPPUNIT_ASSERT_EQUAL(r.first, false);
	CPPUNIT_ASSERT_EQUAL(r.second, sz4::make_nanosecond_time(90, 0));

	r = cache.search_data_left(sz4::make_nanosecond_time(140, 0), 
					sz4::make_nanosecond_time(130, 0), 
					test_search_condition(10));
	CPPUNIT_ASSERT_EQUAL(r.first, false);
	CPPUNIT_ASSERT_EQUAL(r.second, sz4::make_nanosecond_time(140, 0));

	r = cache.search_data_left(sz4::make_nanosecond_time(110, 0), 
					sz4::make_nanosecond_time(90, 0), 
					test_search_condition(0));
	CPPUNIT_ASSERT_EQUAL(r.first, true);
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sz4::nanosecond_time_t>::is_valid(r.second), false);

	r = cache.search_data_left(sz4::make_nanosecond_time(110, 0), 
					sz4::make_nanosecond_time(80, 0), 
					test_search_condition(0));
	CPPUNIT_ASSERT_EQUAL(r.first, false);
	CPPUNIT_ASSERT_EQUAL(sz4::make_nanosecond_time(90, 0), r.second);

	CPPUNIT_ASSERT_EQUAL(size_t(2), cache.blocks().size());

	sz4::nanosecond_time_t t3 = sz4::make_nanosecond_time(120, 0);
	cache.store_value(30, t3, true);

	CPPUNIT_ASSERT_EQUAL(size_t(1), cache.blocks().size());
}

void Sz4DefinableParamCache::test2() {
	definable_param_cache cache(PT_SEC10, m_cache);
	test_search_condition cond(10);
	int v;
	bool fixed;
	sz4::generic_block_ptr_set set;

	cache.store_value(10, sz4::make_nanosecond_time(100, 0), true);
	CPPUNIT_ASSERT(cache.get_value(sz4::make_nanosecond_time(100, 0), v, fixed));
	CPPUNIT_ASSERT_EQUAL(v, 10);

	cache.store_value(20, sz4::make_nanosecond_time(110, 0), true);
	cache.store_value(10, sz4::make_nanosecond_time(120, 0), true);

	CPPUNIT_ASSERT_EQUAL(size_t(3), cache.blocks().begin()->second->data().size());

	cache.store_value(10,  sz4::make_nanosecond_time(110, 0), true);
	CPPUNIT_ASSERT_EQUAL(size_t(1), cache.blocks().begin()->second->data().size());

	cache.store_value(30, sz4::make_nanosecond_time(140, 0), true);
	cache.store_value(40, sz4::make_nanosecond_time(150, 0), true);
	CPPUNIT_ASSERT_EQUAL(size_t(2), cache.blocks().size());

	cache.store_value(10, sz4::make_nanosecond_time(130, 0), true);
	CPPUNIT_ASSERT_EQUAL(size_t(1), cache.blocks().size());
	CPPUNIT_ASSERT_EQUAL(size_t(3), cache.blocks().begin()->second->data().size());

}

