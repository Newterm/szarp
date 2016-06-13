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

#include "libpar.h"
#include "zmqhandler.h"
#include <event.h>
#include <evdns.h>
#include <iostream>
#include <ctime>

namespace py = boost::python;

namespace szarp {

class ipc {
public:
	ipc() : m_cfg(nullptr), m_ipc(nullptr), m_read(nullptr), m_read_count(0), m_send_count(0), m_zmq(nullptr), m_zmq_ctx(new zmq::context_t(1))
		{}

	DaemonConfig* getConfig() { return m_cfg; }

	int configure(int *argc, char *argv[]);
	int configure_events();

	int get_line_number();
	const std::string& get_ipk_path();

	bool check_for_no_data(size_t index, py::object & val);
	void set_read(size_t index, py::object & val);
	void set_read_sz4_double(size_t index, py::object & val);
	void set_read_sz4_float(size_t index, py::object & val);
	void set_read_sz4_int(size_t index, py::object & val);
	void set_read_sz4_short(size_t index, py::object & val);
	template <typename T> void set_read_sz4(size_t index, T val);
	void set_no_data(size_t index) ;
	int get_send(size_t index) ;

	void go_parcook() ;
	void go_sender() ;
	void go_sz4() ;

	void release_sz3(py::object & period);
	inline void release_sz4() { m_sz4_auto = true; }
	void force_sz4() { m_force_sz4 = true; }
	inline bool sz4_ready() { return m_zmq != nullptr; }
	static void cycle_timer_callback(int fd, short event, void* arg);

	inline std::string get_conf_str() { return m_conf_str; }
	inline zmqhandler* getZmq() { return m_zmq; }

protected:
	DaemonConfig * m_cfg;
	IPCHandler* m_ipc;
	short * m_read;
	short * m_send;
	size_t m_read_count;
	size_t m_send_count;
	std::string m_conf_str;

	bool m_sz3_auto = false;
	bool m_sz4_auto = false;
	bool m_force_sz4 = false;

	zmqhandler * m_zmq;
	zmq::context_t* m_zmq_ctx;

	struct event_base* m_event_base;

	struct event m_timer;
	struct timeval m_cycle;
};

void ipc::release_sz3(py::object & period) {
	m_sz3_auto = true; 
	try {
		m_cycle.tv_sec = (int)py::extract<int>(period);
	} catch (py::error_already_set const &) {
		m_cycle.tv_sec = 10;
	}
}

int ipc::configure_events() {
	m_event_base = (struct event_base*) event_init();
	if (!m_event_base)
		return 1;
	evtimer_set(&m_timer, cycle_timer_callback, this);
	event_base_set(m_event_base, &m_timer);
	return 0;
}

int ipc::configure(int *argc, char *argv[]) {
	m_cfg = new DaemonConfig("pythondmn");

	configure_events();
	if (int ret = m_cfg->Load(argc, argv, 0, NULL, 0, m_event_base))
		return ret;

	m_ipc = new IPCHandler(m_cfg);

	if (!m_cfg->GetSingle()) {
		if (m_ipc->Init())
			return 1;
		sz_log(2, "IPC initialized successfully");
	} else {
		sz_log(2, "Single mode, ipc not intialized!!!");
	}

	m_conf_str = m_cfg->GetPrintableDeviceXMLString();

	TDevice * dev = m_cfg->GetDevice();
	for (TUnit* unit = dev->GetFirstRadio()->GetFirstUnit();
			unit; unit = unit->GetNext()) {

		m_read_count += unit->GetParamsCount();
		m_send_count += unit->GetSendParamsCount();
	}

	m_read = m_ipc->m_read;
	m_send = m_ipc->m_send;

	sz_log(2, "m_read_count: %d, m_send_count: %d", m_read_count, m_send_count);

	if (!m_event_base) return 0;

	m_cycle.tv_sec  = 10;
	m_cycle.tv_usec = 0;

	try {
		char* sub_address = libpar_getpar("parhub", "pub_conn_addr", 1);
		char* pub_address = libpar_getpar("parhub", "sub_conn_addr", 1); // we publish on parhub's subscribe address
		m_zmq = new zmqhandler(m_cfg->GetIPK(), m_cfg->GetDevice(), *m_zmq_ctx, sub_address, pub_address);
		sz_log(2, "ZMQ initialized successfully");
	} catch (zmq::error_t& e) {
		m_zmq = nullptr;
		sz_log(0, "ZMQ not initialized!");
	}

	evtimer_add(&m_timer, &m_cycle);
	return 0;
}

void ipc::cycle_timer_callback(int fd, short event, void* arg) {
	ipc* ipc_ptr = (ipc*) arg;

	if (ipc_ptr->m_sz3_auto && !ipc_ptr->m_force_sz4) ipc_ptr->go_parcook();

	evtimer_add(&ipc_ptr->m_timer, &ipc_ptr->m_cycle);
}

int ipc::get_line_number() {
	return m_cfg->GetLineNumber();
}

const std::string& ipc::get_ipk_path() {
	return m_cfg->GetIPKPath();
}

void ipc::set_read(size_t index, py::object & val) {
	if (m_force_sz4) { 
		set_read_sz4_int(index, val);
		return;
	}

	if (index >= m_read_count) {
		sz_log(7, "ipc::set_read ERROR index (%d) greater than params count (%d)", index, m_read_count);
		return;
	}

	if (Py_None == val.ptr()) {
		sz_log(9, "ipc::set_read got None, setting %d to NO_DATA", index);
		m_read[index] = SZARP_NO_DATA;
		return;
	}

	try {
		m_read[index] = py::extract<int>(val);
		sz_log(9, "ipc::set_read setting value %d to %d", index, m_read[index]);
	} catch (py::error_already_set const &) {
		sz_log(9, "ipc::set_read extract error, setting %d to NO_DATA", index);
		m_read[index] = SZARP_NO_DATA;
		PyErr_Clear();
	}
}

bool ipc::check_for_no_data(unsigned int index, py::object & val) {
	if (Py_None == val.ptr()) {
		time_t timev = time(NULL);
		sz_log(9, "Pythondmn sz4_int got NONE at %d", index);
		if (m_zmq) m_zmq->set_value(index, timev, SZARP_NO_DATA);
		m_read[index] = SZARP_NO_DATA;
		if (m_sz4_auto) go_sz4();
		return true;
	}

	return false;
}


void ipc::set_read_sz4_int(unsigned int index, py::object & val) {
	if (check_for_no_data(index, val)) return;

	try {
		auto got = (int)py::extract<int>(val);
		set_read_sz4<int>(index, got);
	} catch (py::error_already_set const &) {
		time_t timev = time(NULL);
		sz_log(9, "Pythondmn sz4_int extract error, setting %d to NO_DATA", index);
		if (m_zmq) m_zmq->set_value(index, timev, SZARP_NO_DATA);
		if (m_sz4_auto) go_sz4();
		PyErr_Clear();
	}
}

void ipc::set_read_sz4_float(unsigned int index, py::object & val) {
	if (check_for_no_data(index, val)) return;

	try {
		auto got = (float)py::extract<float>(val);
		set_read_sz4<float>(index, got);
	} catch (py::error_already_set const &) {
		time_t timev = time(NULL);
		sz_log(9, "Pythondmn sz4_float extract error, setting %d to NO_DATA", index);
		if (m_zmq) m_zmq->set_value(index, timev, SZARP_NO_DATA);
		if (m_sz4_auto) go_sz4();
		PyErr_Clear();
	}
}

void ipc::set_read_sz4_double(unsigned int index, py::object & val) {
	if (check_for_no_data(index, val)) return;

	try {
		auto got = (double)py::extract<double>(val);
		set_read_sz4<double>(index, got);
	} catch (py::error_already_set const &) {
		time_t timev = time(NULL);
		sz_log(9, "Pythondmn sz4_double extract error, setting %d to NO_DATA", index);
		if (m_zmq) m_zmq->set_value(index, timev, SZARP_NO_DATA);
		if (m_sz4_auto) go_sz4();
		PyErr_Clear();
	}
}

void ipc::set_read_sz4_short(unsigned int index, py::object & val) {
	if (check_for_no_data(index, val)) return;

	try {
		auto got = (short)py::extract<short>(val);
		set_read_sz4<short>(index, got);
	} catch (py::error_already_set const &) {
		time_t timev = time(NULL);
		sz_log(9, "Pythondmn sz4_short extract error, setting %d to NO_DATA", index);
		if (m_zmq) m_zmq->set_value(index, timev, SZARP_NO_DATA);
		if (m_sz4_auto) go_sz4();
		PyErr_Clear();
	}
}

template <typename T> void ipc::set_read_sz4(unsigned int index, T val) {
	if (index >= m_read_count) {
		sz_log(7, "Pythondmn ERROR index (%d) greater than params count (%d)", index, m_read_count);
		return;
	}

	time_t timev = time(NULL);

	if (m_zmq) m_zmq->set_value(index, timev, val);
	if (m_sz4_auto) go_sz4();
}

template void ipc::set_read_sz4<>(unsigned int index, short val);
template void ipc::set_read_sz4<>(unsigned int index, int val);
template void ipc::set_read_sz4<>(unsigned int index, float val);
template void ipc::set_read_sz4<>(unsigned int index, double val);

void ipc::set_no_data(size_t index) {
	if (index >= m_read_count) {
		sz_log(7, "ipc::set_no_data ERROR index (%d) greater than params count (%d)", index, m_read_count);
		return;
	}

	m_read[index] = SZARP_NO_DATA;
}

int ipc::get_send(size_t index) {
	if (index >= m_send_count) {
		sz_log(0, "ipc::set ERROR index (%d) greater than params count (%d)", index, m_send_count);
		return SZARP_NO_DATA;
	}

	sz_log(9, "ipc::set index (%d) val: (%d)", index, m_send[index]);
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

void ipc::go_sz4() {
	if (m_zmq) {
		m_zmq->publish();
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
	szarp::ipc ipc;

	if (ipc.configure(&argc, argv)) {
		sz_log(2, "ipc::configure error -- exiting");
		exit(1);
	}

	DaemonConfig* cfg = ipc.getConfig();

	const char *device_name = strdup(cfg->GetDevicePath());
	if (!device_name) {
		sz_log(2, "No device specified -- exiting");
		exit(1);
	}

	FILE * fp = fopen(device_name, "r");
	if (NULL == fp) {
		sz_log(2, "Script %s doesn't exists -- exiting", device_name);
		exit(1);
	}

	try {

		Py_Initialize();

		py::object main_module(py::handle<>(py::borrowed(PyImport_AddModule("__main__"))));
		py::object main_namespace = main_module.attr("__dict__");

		main_namespace["IPC"] = py::class_<szarp::ipc>("IPC")
			.def("get_line_number", &szarp::ipc::get_line_number)
			.def("get_ipk_path", &szarp::ipc::get_ipk_path, py::return_value_policy<py::copy_const_reference>())
			.def("autoupdate_sz3", &szarp::ipc::release_sz3)
			.def("autopublish_sz4", &szarp::ipc::release_sz4)
			.def("sz4_ready", &szarp::ipc::sz4_ready)
			.def("force_sz4", &szarp::ipc::force_sz4)
			.def("go_sz4", &szarp::ipc::go_sz4)
			.def("set_read", &szarp::ipc::set_read)
			.def("set_read_sz4", &szarp::ipc::set_read_sz4_int)
			.def("set_read_sz4_short", &szarp::ipc::set_read_sz4_short)
			.def("set_read_sz4_int", &szarp::ipc::set_read_sz4_int)
			.def("set_read_sz4_float", &szarp::ipc::set_read_sz4_float)
			.def("set_read_sz4_double", &szarp::ipc::set_read_sz4_double)
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
