#include "szpmap_test.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestSuite.h>
#include <cppunit/ui/text/TestRunner.h>

#include <cstdio>

void SzParamMapTest::set_tests(std::vector<SzParamTest>& test_params) 
{
	for (SzParamTest tp : test_params) {
		SzParamMap::SzParam szp(tp.address, tp.type, tp.val_op, tp.prec);
		if (tp.has_min) {
			szp.setMin(tp.min); 
			szp.setHasMin(true);
		}
		if (tp.has_max) {
			szp.setMax(tp.max);
			szp.setHasMax(true);
		}

		szpmap_.AddParam(tp.id, szp);
	
		tests_.push_back([this,tp]() {
			for (ValueTest vt : tp.value_tests) {
				
				char buffer[100];
				sprintf(buffer, "SzParamMap id:%lu SzParam(%d,%s,%s,%d) Range: [%.15f,%.15f] value: %.15f", 
						tp.id,
						tp.address, 
						tp.type.c_str(), 
						tp.val_op.c_str(),
						tp.prec,
						tp.min,
						tp.max,
						vt.value);

				if (vt.positive) {
					CPPUNIT_ASSERT_MESSAGE(buffer, szpmap_.out_of_range(vt.value, tp.id));
				} else {
					CPPUNIT_ASSERT_MESSAGE(buffer, !szpmap_.out_of_range(vt.value, tp.id));
				}
			}
		});
	}
}

void SzParamMapTest::setUp() 
{	
	std::vector<SzParamTest> test_params = {
		// id, address, type, val_op, prec, min, max, has_min, has_max
		{0, 0, "integer", "", 0, 0, 0, false, false,
			{	
				{0,false},
				{5,false},
				{9,false},
				{-9,false}
			}
		},
		{1, 1, "integer", "", 0, 5.25, 0, true, false,
			{	
				{0,true},
				{5,false},
				{6,false}
			}
		},
		{2, 2, "integer", "", 0, 0, 10.0025, false, true,
			{	
				{11,true},
				{-11,false},
				{10,false},
				{-10,false},
			}
		},
		{3, 3, "integer", "", 0, 5.25, 10.0025, true, true,
			{		
				{0,true},
				{5,false},
				{6,false},
				{11,true},
				{-11,true},
				{10,false},
				{-10,true},
			}
		},
		{4, 4, "real", "", 8, 0, 0, false, false,
			{	
				{0.00000000,false},
				{5.25000000,false},
				{5.24999999,false},
				{5.24999999,false}
			}
		},
		{5, 5, "real", "", 8, 5.25, 0, true, false,
			{	
				{0.00000000,true},
				{5.25000000,false},
				{5.24999999,true},
				{5.24999998,true},
				{5.24999997,true}
			}
		},
		{6, 6, "real", "", 8, 5.25, 10.0025, true, true,
			{
				{0.00000000,true},
				{5.25000000,false},
				{5.24999999,true},
				{5.24999998,true},
				{5.24999997,true},
				{11.00000000,true},
				{10.00250000,false},
				{10.00250001,true},
				{10.00250002,true},
				{10.00250003,true},
				{5.249999999999,false} 
			}
		},
		{7, 7, "real", "", 4, 0.0000, 1.1111, true, true,
			{
				{0.0000,false},
				{-0.0001,true},
				{-0.0002,true},
				{-0.0003,true},
				{0.0001,false},
				{0.0002,false},
				{0.0003,false},
				{1.1111, false},
				{1.1112, true},
				{1.1113, true},
				{1.1114, true},
				{1.1110, false},
				{1.1109, false},
				{1.1108, false},
				{1.111099999, false}
			}
		},
		{8, 8, "real", "", 8, 5.2499999999, 10.1111, true, true,
			{
				{5.25,false},
				{5.24999999990, false},
				{5.24999999999, false},
				{5.24999999000, true}
			}
		},
		{9, 9, "real", "", 4, 0, 2.5, true, true,
			{
				{0.0000,false},
				{0.00001,false},
				{0.000001,false},
				{0.0000001,false},
				{0.00000001,false},
				{0.000000001,false},
				{0.0000000001,false},
				{0.00000000001,false},
				{0.000000000001,false},
				{0.0000000000001,false},
				{0.00000000000001,false},
				{0.000000000000001,false},
				{0.0000000000000001,false},
				{0.00000000000000001,false}
			}
		},
		{10, 10, "real", "", 4, -5.5, 0, true, true,
			{
				{-0.0001,false},
				{-0.00001,false},
				{-0.000001,false},
				{-0.0000001,false},
				{-0.00000001,false},
				{-0.000000001,false},
				{-0.0000000001,false},
				{-0.00000000001,false},
				{-0.000000000001,false},
				{-0.0000000000001,false},
				{-0.00000000000001,false},
				{-0.000000000000001,false},
				{-0.0000000000000001,false},
				{-0.00000000000000001,false},
				{0.0000,false},
				{0.0001,true},
				{0.00001,false},
				{0.000001,false},
				{0.0000001,false},
				{0.00000001,false},
				{0.000000001,false},
				{0.0000000001,false},
				{0.00000000001,false},
				{0.000000000001,false},
				{0.0000000000001,false},
				{0.00000000000001,false},
				{0.000000000000001,false},
				{0.0000000000000001,false},
				{0.00000000000000001,false}
			}
		}
	};
	set_tests(test_params);
}

void SzParamMapTest::TestOutOfRange()
{
	for (auto test : tests_) test();
}

void SzParamMapTest::tearDown()
{}

int main() 
{
	CppUnit::TextUi::TestRunner runner;
	CppUnit::TestResult result;
	runner.addTest(SzParamMapTest::suite());
	runner.run();
}

