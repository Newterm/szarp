#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>
#include <boost/filesystem.hpp>
#include "../wx/common/user_def_ipk.h"

class NullLogHandler: public szlog::LogHandler {
public:
	void log(const std::string& msg, szlog::priority p = szlog::priority::info) const override {}
	void log(const char* msg, szlog::priority p = szlog::priority::info) const override {}

};

class IpkContainerTest: public CPPUNIT_NS::TestFixture
{
	std::wstring base_data_name;
	std::wstring base_name = L"testbase";
	std::unique_ptr<UserDefinedIPKManager> ipk_manager;
	UserDefinedIPKContainer* ipk;

public:
	void smoke_test1();
	void setUp();
	void tearDown();

	CPPUNIT_TEST_SUITE( IpkContainerTest );
	CPPUNIT_TEST( smoke_test1 );
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION( IpkContainerTest );

void IpkContainerTest::setUp() {
	auto dh = new NullLogHandler();
	std::shared_ptr<NullLogHandler> lh(dh);
	szlog::log().set_logger(lh);

	std::wstringstream ws_base_data_name;
	ws_base_data_name << L"/tmp/ipkcontainer_test." << getpid() << L"." << time(NULL) << L".tmp";
	base_data_name = ws_base_data_name.str();

	boost::filesystem::wpath path(base_data_name + L"/" + base_name);
	boost::filesystem::create_directories(path / L"config");

	ipk_manager.reset(new UserDefinedIPKManager(base_data_name, L"/opt/szarp", L"pl"));
	ipk = ipk_manager->GetIPKContainer();

#if BOOST_FILESYSTEM_VERSION == 3
	std::ofstream ofs((path / L"config/params.xml").native().c_str(), std::ios_base::binary);
#else
	std::ofstream ofs((path / L"config/params.xml").external_file_string().c_str(), std::ios_base::binary);
#endif

	ofs << 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<params xmlns=\"http://www.praterm.com.pl/SZARP/ipk\" version=\"1.0\" read_freq=\"10\" send_freq=\"10\" title=\"Globalmalt Bydgoszcz\" documentation_base_url=\"http://www.szarp.com\">"
"  <device xmlns:modbus=\"http://www.praterm.com.pl/SZARP/ipk-extra\" daemon=\"/dev/null\">"
"    <unit id=\"1\" type=\"1\" subtype=\"1\" bufsize=\"1\">"
"    	<param name=\"a:a:a\" data_type=\"uinteger\" short_name=\"-\" unit=\"°C\" prec=\"1\" draw_name=\"\" base_ind=\"auto\"/>"
"    	<param name=\"b:b:b\" data_type=\"uinteger\" short_name=\"-\" unit=\"°C\" prec=\"1\" draw_name=\"\" base_ind=\"auto\"/>"
"    </unit>"
"  </device>"
"</params>";
	ofs.close();
}

void IpkContainerTest::tearDown() {
	IPKContainer::Destroy();
	boost::filesystem::remove_all(base_data_name);
}

void IpkContainerTest::smoke_test1() {
	CPPUNIT_ASSERT(ipk_manager);
	CPPUNIT_ASSERT(ipk);

	/* Getting and caching configs */
	CPPUNIT_ASSERT(ipk->GetConfig(L"nonexistent") == nullptr);
	CPPUNIT_ASSERT(ipk->GetConfig(base_name) != nullptr); // was not added yet so should be nullptr

	/* Getting params UC/WC */
	CPPUNIT_ASSERT(ipk->GetParam(L"nonexistent:a:a:a", false) == nullptr);
	CPPUNIT_ASSERT(ipk->GetParam(L"nonexistent:a:a:a", true) == nullptr);
	CPPUNIT_ASSERT(ipk->GetParam(L"nonexistent:a:a:a", false) == nullptr);
	CPPUNIT_ASSERT(ipk->GetParam(SC::S2U(L"nonexistent:a:a:a"), true) == nullptr);
	CPPUNIT_ASSERT(ipk->GetParam(base_name + L":nonexistent:a:a", false) == nullptr);
	CPPUNIT_ASSERT(ipk->GetParam(base_name + L":nonexistent:a:a", true) == nullptr);
	CPPUNIT_ASSERT(ipk->GetParam(SC::S2U(base_name + L":nonexistent:a:a"), false) == nullptr);
	CPPUNIT_ASSERT(ipk->GetParam(SC::S2U(base_name + L":nonexistent:a:a"), true) == nullptr);

	auto param_a = ipk->GetParam(base_name + L":a:a:a", false);
	CPPUNIT_ASSERT(param_a != nullptr);
	CPPUNIT_ASSERT(param_a == ipk->GetParam(base_name + L":a:a:a", true));
	CPPUNIT_ASSERT(param_a == ipk->GetParam(SC::S2U(base_name + L":a:a:a")));

	auto param_b = ipk->GetParam(base_name + L":b:b:b", false);
	CPPUNIT_ASSERT(param_a != param_b);

	/* Managing user defined */
	TParam *defp = new TParam(nullptr);
	std::wstring d_name = L"user:defined:1";
	defp->Configure(d_name, L"", L"", L"", NULL);

	ipk->AddUserDefined(L"nonexistent", defp);
	CPPUNIT_ASSERT_EQUAL(ipk->GetParam(L"nonexistent:" + d_name), (TParam*) nullptr);

	ipk->AddUserDefined(base_name, defp);
	CPPUNIT_ASSERT_EQUAL(ipk->GetParam(base_name + L":" + d_name), defp);

	ipk->RemoveUserDefined(base_name, defp);
	CPPUNIT_ASSERT_EQUAL(ipk->GetParam(base_name + L":" + d_name), (TParam*) nullptr);
}
