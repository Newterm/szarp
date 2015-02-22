#include <boost/python.hpp>
#include "szarp_config.h"
#include "szbase/szbbase.h"
#include "conversion.h"

namespace libpyszbase {

bool g_initialized = false;

Szbase* get_szbase_object() {
	if (!g_initialized)
		throw std::runtime_error("libpyszbase library not initialized");

	Szbase* szbase = Szbase::GetObject();	
	szbase->NextQuery();

	return szbase;
}

void check_no_init() {
	if (g_initialized)
		throw std::runtime_error("libpyszbase already initialized");
}

void check_init() {
	if (!g_initialized)
		throw std::runtime_error("libpyszbase not initialized");
}

void init(const std::wstring& szarp_path, const std::wstring& lang) {
	check_no_init();

	IPKContainer::Init(szarp_path, szarp_path, lang);
	Szbase::Init(szarp_path, false);

	g_initialized = true;
}

void shutdown() {
	check_init();

	Szbase::Destroy();	
	IPKContainer::Destroy();

	g_initialized = false;
}

double get_value(const std::wstring& param, time_t time, SZARP_PROBE_TYPE probe_type) {
	Szbase *szbase = get_szbase_object();

	bool is_fixed, ok;
	std::wstring error;
	double ret = szbase->GetValue(param, time, probe_type, 0, &is_fixed, ok, error);
	
	if (!ok)
		throw std::runtime_error(SC::S2A(error));

	return ret;
}

time_t search_first(const std::wstring &param) {
	Szbase *szbase = get_szbase_object();

	bool ok;
	time_t ret = szbase->SearchFirst(param, ok);
	if (!ok)
		throw std::runtime_error("Param " + SC::S2A(param) + " not found");

	return ret;
}

time_t search_last(const std::wstring &param) {
	Szbase *szbase = get_szbase_object();

	bool ok;
	time_t ret = szbase->SearchLast(param, ok);
	if (!ok)
		throw std::runtime_error("Param " + SC::S2A(param) + " not found");

	return ret;
}

time_t search(const std::wstring &param, time_t start, time_t end, int direction, SZARP_PROBE_TYPE probe) {
	Szbase *szbase = get_szbase_object();

	bool ok = true;
	std::wstring error;
	time_t ret = szbase->Search(param, start, end, direction, probe, ok, error);
	if (!ok)
		throw std::runtime_error(SC::S2A(error));

	return ret;
}

void set_prober_server_address(const std::wstring &prefix, const std::wstring& address, const std::wstring& port) {
	Szbase *szbase = get_szbase_object();
	szbase->SetProberAddress(prefix, address, port);
}

}

BOOST_PYTHON_MODULE(libpyszbase)
{
	using namespace boost::python;

	enum_<SZARP_PROBE_TYPE>("PROBE_TYPE")
		.value("PT_SEC10", PT_SEC10)
		.value("PT_MIN10", PT_MIN10)
		.value("PT_HOUR", PT_HOUR)
		.value("PT_HOUR8", PT_HOUR8)
		.value("PT_DAY", PT_DAY)
		.value("PT_WEEKP", PT_WEEK)
		.value("PT_MONTH", PT_MONTH)
		.value("PT_YEAR", PT_YEAR)
		;


	def("init", libpyszbase::init);
	def("shutdown", libpyszbase::shutdown);
	def("get_value", libpyszbase::get_value);
	def("search_first", libpyszbase::search_first);
	def("search_last", libpyszbase::search_last);
	def("search", libpyszbase::search);
	def("set_prober_server_address", libpyszbase::set_prober_server_address);
}

