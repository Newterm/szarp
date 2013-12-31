#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <boost/python.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/return_value_policy.hpp>

#include "conversion.h"
#include "dmncfg.h"
#include "ipchandler.h"
#include "liblog.h"

namespace py = boost::python;

namespace szarp {
    class ipc {
	public:
	    ipc() : m_cfg(NULL), m_ipc(NULL), m_read(NULL), m_read_count(0) {};

	    int configure(DaemonConfig * cfg);

	    void set_read(size_t index, py::object & val) ;
	    void set_no_data(size_t index) ;
	    int get_send(size_t index) ;

	    void go_parcook() ;
	    void go_sender() ;

	    DaemonConfig * m_cfg;
	    IPCHandler* m_ipc;
	    short * m_read;
	    short * m_send;
	    size_t m_read_count;
	    size_t m_send_count;
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

	TDevice * dev = m_cfg->GetDevice();
	TUnit* unit = dev->GetFirstRadio()->GetFirstUnit();
	m_read_count = unit->GetParamsCount();
	m_send_count = unit->GetSendParamsCount();

	m_read = m_ipc->m_read;
	m_send = m_ipc->m_send;

	sz_log(1, "m_read_count: %d, m_send_count: %d", m_read_count, m_send_count);

	return 0;
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
	}
	catch (py::error_already_set const &) {
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
}

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

    try {
	Py_Initialize();

	szarp::ipc ipc;

	if (ipc.configure(m_cfg)) {
	    sz_log(0, "ipc::configure error -- exiting");
	    exit(1);
	}

	FILE * fp = fopen(device_name, "r");
	if (NULL == fp) {
		sz_log(0, "Script %s doesn't exists -- exiting", device_name);
		exit(1);
	}


	py::object main_module(py::handle<>(py::borrowed(PyImport_AddModule("__main__"))));

	py::object main_namespace = main_module.attr("__dict__");
	main_namespace["IPC"] = py::class_<szarp::ipc>("IPC")
	    .def("set_read", &szarp::ipc::set_read)
	    .def("set_no_data", &szarp::ipc::set_no_data)
	    .def("get_send", &szarp::ipc::get_send)
	    .def("go_sender", &szarp::ipc::go_sender)
	    .def("go_parcook", &szarp::ipc::go_parcook);
	main_namespace["ipc"] = py::ptr(&ipc);

	py::handle<> ignored(( PyRun_File(fp, device_name,
                                     Py_file_input,
                                     main_namespace.ptr(),
                                     main_namespace.ptr() ) ));
    } catch( py::error_already_set ) {
	PyErr_Print();
    }
}
