
class IpkMock: public IPKContainer {
public:
	template<class T> TParam* GetParam(const std::basic_string<T>& global_param_name, bool add_config_if_not_present)
	{
		return new TParam(nullptr, nullptr, 
};

class DefdmnTest: public CppUnit::TestFixture, public Defdmn {
public:
	DefdmnTest(): CppUnit::TestFixture(), Defdmn() {
		m_ipk = new IpkMock();
	}

	void setUp() {
		
	}

	void tearDown() {}

	void test() {
		TParam* param_calc = new TParam(nullptr, nullptr, "v = 2 * 3;", FormulaType::LUA_VA);
		auto params_dependent = get_params_to_observe(param_calc);
		CPPUNIT_ASSERT( params_dependent.size() == 0 );

	}
};
