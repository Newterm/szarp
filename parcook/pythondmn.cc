#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <boost/python.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/return_value_policy.hpp>

#include "conversion.h"
#include "dmncfg.h"
#include "ipchandler.h"
#include "liblog.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"

namespace py = boost::python;

namespace szarp {

class ipc {
public:
	ipc() : m_cfg(nullptr), m_ipc(nullptr), m_read(nullptr),
		m_read_count(0), m_send_count(0)
	{}

	int configure(DaemonConfig * cfg);

	int get_line_number();
	const std::string& get_ipk_path();

	void set_read(size_t index, py::object & val) ;
	void set_no_data(size_t index) ;
	int get_send(size_t index) ;

	void go_parcook() ;
	void go_sender() ;

	std::string get_conf_str()
	{
		return m_conf_str;
	}

protected:
	DaemonConfig * m_cfg;
	IPCHandler* m_ipc;
	short * m_read;
	short * m_send;
	size_t m_read_count;
	size_t m_send_count;
	std::string m_conf_str;
};


int ipc::configure(DaemonConfig * cfg) {
	m_cfg = cfg;
	m_ipc = new IPCHandler(m_cfg);

	if (!m_cfg->GetSingle()) {
		if (m_ipc->Init())
			return 1;
		sz_log(10, "IPC initialized successfully");
	} else {
		sz_log(10, "Single mode, ipc not intialized!!!");
	}

	m_conf_str = cfg->GetPrintableDeviceXMLString();

	TDevice * dev = m_cfg->GetDevice();
	for (TUnit* unit = dev->GetFirstRadio()->GetFirstUnit();
			unit; unit = unit->GetNext()) {

		m_read_count += unit->GetParamsCount();
		m_send_count += unit->GetSendParamsCount();
	}

	m_read = m_ipc->m_read;
	m_send = m_ipc->m_send;

	sz_log(1, "m_read_count: %d, m_send_count: %d", m_read_count, m_send_count);

	return 0;
}

int ipc::get_line_number() {
	return m_cfg->GetLineNumber();
}

const std::string& ipc::get_ipk_path() {
	return m_cfg->GetIPKPath();
}

void ipc::set_read(size_t index, py::object & val) {
	if (index >= m_read_count) {
		sz_log(0, "ipc::set_read ERROR index (%d) greater than params count (%d)", index, m_read_count);
		return;
	}

	if (Py_None == val.ptr()) {
		sz_log(1, "ipc::set_read got None, setting %d to NO_DATA", index);
		m_read[index] = SZARP_NO_DATA;
		return;
	}

	try {
		m_read[index] = py::extract<int>(val);
		sz_log(2, "ipc::set_read DEBUG setting value %d to %d", index, m_read[index]);
	} catch (py::error_already_set const &) {
		sz_log(1, "ipc::set_read extract error, setting %d to NO_DATA", index);
		m_read[index] = SZARP_NO_DATA;
		PyErr_Clear();
	}
}

void ipc::set_no_data(size_t index) {
	if (index >= m_read_count) {
		sz_log(0, "ipc::set_no_data ERROR index (%d) greater than params count (%d)", index, m_read_count);
		return;
	}

	m_read[index] = SZARP_NO_DATA;
}

int ipc::get_send(size_t index) {
	if (index >= m_send_count) {
		sz_log(0, "ipc::set ERROR index (%d) greater than params count (%d)", index, m_send_count);
		return SZARP_NO_DATA;
	}

	sz_log(0, "ipc::set DEBUG index (%d) val: (%d)", index, m_send[index]);
	return m_send[index];
}

void ipc::go_parcook() {
	if (m_ipc) {
		m_ipc->GoParcook();
	}
}

void ipc::go_sender() {
	if (m_ipc) {
		m_ipc->GoSender();
	}
}


class pyszbase {
public:
	pyszbase() : m_initialized(false) {};

	void init(const std::wstring& szarp_path, const std::wstring& lang);
	void shutdown();

	double get_value(const std::wstring& param, time_t time, SZARP_PROBE_TYPE probe_type);
	time_t search_first(const std::wstring &param);
	time_t search_last(const std::wstring &param);
	time_t search(const std::wstring &param, time_t start, time_t end, int direction, SZARP_PROBE_TYPE probe);
	void set_prober_server_address(const std::wstring &prefix, const std::wstring& address, const std::wstring& port);

	Szbase* get_szbase_object();
	void check_no_init();

	bool m_initialized;
};

Szbase* pyszbase::get_szbase_object() {
	if (!m_initialized)
		throw std::runtime_error("libpyszbase library not initialized");

	Szbase* szbase = Szbase::GetObject();	
	szbase->NextQuery();

	return szbase;
}

void pyszbase::check_no_init() {
	if (m_initialized)
		throw std::runtime_error("libpyszbase already initialized");
}

void pyszbase::init(const std::wstring& szarp_path, const std::wstring& lang) {
	check_no_init();

	IPKContainer::Init(szarp_path, szarp_path, lang);
	Szbase::Init(szarp_path, false);

	m_initialized = true;
}

void pyszbase::shutdown() {
	check_no_init();

	Szbase::Destroy();	
	IPKContainer::Destroy();

	m_initialized = false;
}

double pyszbase::get_value(const std::wstring& param, time_t time, SZARP_PROBE_TYPE probe_type) {
	Szbase *szbase = get_szbase_object();

	bool is_fixed, ok;
	std::wstring error;
	double ret = szbase->GetValue(param, time, probe_type, 0, &is_fixed, ok, error);

	if (!ok)
		std::runtime_error(SC::S2A(error));

	return ret;
}

time_t pyszbase::search_first(const std::wstring &param) {
	Szbase *szbase = get_szbase_object();

	bool ok;
	time_t ret = szbase->SearchFirst(param, ok);
	if (!ok)
		throw std::runtime_error("Param " + SC::S2A(param) + " not found");

	return ret;
}

time_t pyszbase::search_last(const std::wstring &param) {
	Szbase *szbase = get_szbase_object();

	bool ok;
	time_t ret = szbase->SearchLast(param, ok);
	if (!ok)
		throw std::runtime_error("Param " + SC::S2A(param) + " not found");

	return ret;
}

time_t pyszbase::search(const std::wstring &param, time_t start, time_t end,
		int direction, SZARP_PROBE_TYPE probe) {

	Szbase *szbase = get_szbase_object();

	bool ok = true;
	std::wstring error;
	time_t ret = szbase->Search(param, start, end, direction, probe, ok, error);
	if (!ok)
		throw std::runtime_error(SC::S2A(error));

	return ret;
}

void pyszbase::set_prober_server_address(const std::wstring &prefix,
		const std::wstring& address, const std::wstring& port) {
	Szbase *szbase = get_szbase_object();
	szbase->SetProberAddress(prefix, address, port);
}


}	// namespace szarp


int main( int argc, char ** argv )
{
	DaemonConfig* m_cfg;

	m_cfg = new DaemonConfig("pythondmn");
	if (m_cfg->Load(&argc, argv))
		return 1;

	const char *device_name = strdup(m_cfg->GetDevicePath());
	if (!device_name) {
		sz_log(0, "No device specified -- exiting");
		exit(1);
	}

	FILE * fp = fopen(device_name, "r");
	if (NULL == fp) {
		sz_log(0, "Script %s doesn't exists -- exiting", device_name);
		exit(1);
	}

	try {
		Py_Initialize();

		py::object main_module(py::handle<>(py::borrowed(PyImport_AddModule("__main__"))));
		py::object main_namespace = main_module.attr("__dict__");

		szarp::ipc ipc;

		if (ipc.configure(m_cfg)) {
			sz_log(0, "ipc::configure error -- exiting");
			exit(1);
		}

		main_namespace["IPC"] = py::class_<szarp::ipc>("IPC")
			.def("get_line_number", &szarp::ipc::get_line_number)
			.def("get_ipk_path", &szarp::ipc::get_ipk_path, py::return_value_policy<py::copy_const_reference>())
			.def("set_read", &szarp::ipc::set_read)
			.def("set_no_data", &szarp::ipc::set_no_data)
			.def("get_send", &szarp::ipc::get_send)
			.def("go_sender", &szarp::ipc::go_sender)
			.def("go_parcook", &szarp::ipc::go_parcook)
			.def("get_conf_str", &szarp::ipc::get_conf_str);
		main_namespace["ipc"] = py::ptr(&ipc);


		szarp::pyszbase pyszbase;

		py::object pyszbase_class = py::class_<szarp::pyszbase>("PySzbase")
			.def("init", &szarp::pyszbase::init)
			.def("shutdown", &szarp::pyszbase::shutdown)
			.def("get_value", &szarp::pyszbase::get_value)
			.def("search_first", &szarp::pyszbase::search_first)
			.def("search_last", &szarp::pyszbase::search_last)
			.def("search", &szarp::pyszbase::search)
			.def("set_prober_server_address", &szarp::pyszbase::set_prober_server_address)
			;
		main_namespace["PySzbase"] = pyszbase_class;
		main_namespace["pyszbase"] = py::ptr(&pyszbase);

		py::scope pyszbase_scope = pyszbase_class;

		py::enum_<SZARP_PROBE_TYPE>("PROBE_TYPE")
			.value("PT_SEC10", PT_SEC10)
			.value("PT_MIN10", PT_MIN10)
			.value("PT_HOUR", PT_HOUR)
			.value("PT_HOUR8", PT_HOUR8)
			.value("PT_DAY", PT_DAY)
			.value("PT_WEEKP", PT_WEEK)
			.value("PT_MONTH", PT_MONTH)
			.value("PT_YEAR", PT_YEAR)
			;

		py::handle<> ignored(( PyRun_File(fp, device_name,
										Py_file_input,
										main_namespace.ptr(),
										main_namespace.ptr() ) ));
	} catch( py::error_already_set ) {
		PyErr_Print();
	}
}
