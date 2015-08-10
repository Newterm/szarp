#include "config.h"

#include "sz4/defs.h"
#include <cppunit/extensions/HelperMacros.h>

class Sz4WeightedSumTest : public CPPUNIT_NS::TestFixture
{
	void test();

	CPPUNIT_TEST_SUITE( Sz4WeightedSumTest );
	CPPUNIT_TEST( test );
	CPPUNIT_TEST_SUITE_END();
};

void Sz4WeightedSumTest::test() {
	typedef sz4::weighted_sum<short, sz4::second_time_t> type;
	type w;

	typedef type::time_diff_type diff_t;
	typedef type::sum_type sum_t;

	CPPUNIT_ASSERT(w.fixed());

	sum_t s;
	diff_t d;

	s = w.sum(d);
	CPPUNIT_ASSERT_EQUAL(diff_t(0), d);

	w.add(5, 2);
	s = w.sum(d);
	CPPUNIT_ASSERT_EQUAL(diff_t(2), d);
	CPPUNIT_ASSERT_EQUAL(sum_t(10), s);

	w.add(5, 1);
	s = w.sum(d);
	CPPUNIT_ASSERT_EQUAL(diff_t(3), d);
	CPPUNIT_ASSERT_EQUAL(sum_t(15), s);

	w.add(5, 4);
	s = w.sum(d);
	CPPUNIT_ASSERT_EQUAL(diff_t(7), d);
	CPPUNIT_ASSERT_EQUAL(sum_t(35), s);
}

CPPUNIT_TEST_SUITE_REGISTRATION( Sz4WeightedSumTest );
