#include "integrator.h"
#include <cmath>
#include <cppunit/extensions/HelperMacros.h>

class IntegratorTestFixture : public CPPUNIT_NS::TestFixture
{
	void simpleTest();

	CPPUNIT_TEST_SUITE( IntegratorTestFixture );
	CPPUNIT_TEST( simpleTest );
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
};

CPPUNIT_TEST_SUITE_REGISTRATION( IntegratorTestFixture );

void IntegratorTestFixture::setUp() {
	Integrator::setMaxEntriesPerParam(2);
}

void IntegratorTestFixture::simpleTest() {
	// vector index == time, from 0
	const std::map<const std::string, const std::vector<double>> values = {
		{"first", {
		-1, -1, 1, 1, 1,
		2, 2, 2, -1, -1,
		-1, 4, 4, 4, -1
		}},
		{"second", {
		-1, -1, 5 , 7, -1,
		-1, -1, 7 , -1, -1,
		-1, -1, 10503599627370496L, 10503599627370496L, 10503599627370496L	// > 2^52
		}}
	};

	int fetch_count = 0;
	auto data_provider = [&values, &fetch_count](const std::string& param_name, time_t probe_time) -> double {
		fetch_count++;
		return values.at(param_name).at(probe_time);
	};

	auto time_mover = [](time_t start_time, int steps) -> time_t {
		return start_time + steps;
	};

	const double nodata_value = -1;
	const double delta = 0.0001;
	auto is_no_data = [nodata_value, delta](double value) -> bool {
		return fabs(value - nodata_value) < delta;
	};
	Integrator integrator(data_provider, time_mover, is_no_data, nodata_value);

	auto get_integral = [&fetch_count, &integrator](const std::string& param_name, time_t start_time, time_t end_time) -> double {
		fetch_count = 0;
		return integrator.GetIntegral(param_name, start_time, end_time);
	};

	CPPUNIT_ASSERT_EQUAL(0, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(nodata_value, get_integral("first", 0, 0), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(nodata_value, get_integral("first", 0, 1), delta);
	CPPUNIT_ASSERT_EQUAL(2, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(nodata_value, get_integral("first", 1, 1), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(nodata_value, get_integral("first", 1, 1), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);	//TODO : this could be 0!
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0, get_integral("first", 2, 2), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(1, get_integral("first", 2, 3), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, get_integral("first", 2, 4), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);

	CPPUNIT_ASSERT_DOUBLES_EQUAL(nodata_value, get_integral("first", 1, 1), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, get_integral("first", 2, 4), delta);
	CPPUNIT_ASSERT_EQUAL(0, fetch_count);

	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, get_integral("first", 0, 4), delta);
	CPPUNIT_ASSERT_EQUAL(5, fetch_count);

	// test multiple entries cache
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, get_integral("first", 0, 4), delta);
	CPPUNIT_ASSERT_EQUAL(0, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, get_integral("first", 2, 4), delta);
	CPPUNIT_ASSERT_EQUAL(0, fetch_count);

	CPPUNIT_ASSERT_DOUBLES_EQUAL(4, get_integral("first", 5, 7), delta);
	CPPUNIT_ASSERT_EQUAL(3, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(4, get_integral("first", 5, 8), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(4, get_integral("first", 5, 9), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(4, get_integral("first", 5, 10), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(4 + 4 * 3, get_integral("first", 5, 11), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(4 + 4 * 3 + 4, get_integral("first", 5, 12), delta);
	CPPUNIT_ASSERT_EQUAL(1, fetch_count);

	// test multiple entries cache and purging oldest entries
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, get_integral("first", 2, 4), delta);
	CPPUNIT_ASSERT_EQUAL(0, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(4 + 4 * 3 + 4, get_integral("first", 5, 12), delta);
	CPPUNIT_ASSERT_EQUAL(0, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, get_integral("first", 0, 4), delta);	// this entry has oldest last-use time, so it should be absent already
	CPPUNIT_ASSERT_EQUAL(5, fetch_count);

	CPPUNIT_ASSERT_DOUBLES_EQUAL(8, get_integral("first", 10, 14), delta);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(12, get_integral("first", 7, 11), delta);	// interpolate between two valid points
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2 + 1.5 + 4 + 3 * 4, get_integral("first", 0, 11), delta);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2 + 1.5 + 4, get_integral("first", 0, 10), delta);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2 + 1.5 + 4 + 3 * 4, get_integral("first", 0, 11), delta);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(6, get_integral("second", 2, 3), delta);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(1, get_integral("first", 2, 3), delta);
	CPPUNIT_ASSERT_EQUAL((long long)21007199254740992L, (long long)get_integral("second", 12, 14));

	fetch_count = 0;
	Integrator::Cache cache;
	Integrator cached_integrator1(data_provider, time_mover, is_no_data, nodata_value, cache);
	CPPUNIT_ASSERT_EQUAL(0, fetch_count);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, cached_integrator1.GetIntegral("first", 0, 4), delta);
	CPPUNIT_ASSERT_EQUAL(5, fetch_count);

	Integrator cached_integrator2(data_provider, time_mover, is_no_data, nodata_value, cache);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, cached_integrator2.GetIntegral("first", 0, 4), delta);
	CPPUNIT_ASSERT_EQUAL(5, fetch_count);
}
