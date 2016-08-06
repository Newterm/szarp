#ifndef __SZPMAP_TEST_H__
#define __SZPMAP_TEST_H__

#include <string>
#include <vector>
#include <utility>
#include <functional>

#include <cppunit/extensions/HelperMacros.h>

#include "szpmap.h"

class SzParamMapTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(SzParamMapTest);
	CPPUNIT_TEST(TestOutOfRange);
	CPPUNIT_TEST_SUITE_END();

public:
	struct ValueTest {
		double value;
		bool positive;
	};

	struct SzParamTest {
		unsigned long int id;

		int address;
		std::string type;
		std::string val_op;
		int prec;

		double min;
		double max;
		bool has_min;
		bool has_max;

		std::vector<ValueTest> value_tests;
	};

	void setUp() override;
	void tearDown() override;

	void TestOutOfRange();

private:
	void set_tests(std::vector<SzParamTest>& test_params);

	SzParamMap szpmap_;
	std::vector<std::function<void()>> tests_;
};

#endif /*__SZPMAP_TEST_H__*/
