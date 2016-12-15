#include <cppunit/extensions/HelperMacros.h>

#include "szarp_config.h"

#include "sz4/time.h"
#include "sz4/factory.h"

class Sz4Factory : public CPPUNIT_NS::TestFixture
{
	void nanosecondDoubleTest();

	std::unique_ptr<TParam> param;

	CPPUNIT_TEST_SUITE( Sz4Factory );
	CPPUNIT_TEST( nanosecondDoubleTest );

	CPPUNIT_TEST_SUITE_END();
public:
};

//base class - used by 'types agnostic' parts of the code
class base_class
{
public:
	virtual void method() = 0;
	virtual ~base_class() {};
};

//the real implementation that is a template parametrized with data type and time type
template<class data_type, class time_type> class derived_class : public base_class
{
public:
	derived_class(int some_sample_arg) {};
	void method() override { CPPUNIT_ASSERT(false); }
};
//We have a test verifying that if derived_class for TParam::DOUBLE and TParam::NANOSECOND
//configuration values is created, the corresponding c++ types are used to instantiate the template.
//Specialize ::method for those types to assert(true) (generic case asserts(false))
//in order to check that.
template<> void derived_class<double, sz4::nanosecond_time_t>::method() {
	CPPUNIT_ASSERT(true);
}

//the guy which creates derived object, sz4::factory specializes it with proper types
class builder
{
public:
	template<class data_type, class time_type> static base_class* op(int arg)
	{
		return new derived_class<data_type, time_type>(arg);
	}
};

void Sz4Factory::nanosecondDoubleTest() {
	param.reset(new TParam(nullptr, nullptr));
	param->SetDataType(TParam::DOUBLE);
	param->SetTimeType(TParam::NANOSECOND);

	base_class* b = sz4::factory<base_class, builder>::op(param.get(), 123);
	b->method();

	delete b;
}

CPPUNIT_TEST_SUITE_REGISTRATION( Sz4Factory );
