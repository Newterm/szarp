#include <boost/python.hpp>

namespace py = boost::python;

namespace szarp {
    class ipc {
	public:
	    ipc() : m_val(10) {};
	    int getRead() {
		return m_val;
	    }

	    void setRead(int val) {
		m_val = val;
	    }

	    int m_val;
    };
}

int main( int argc, char ** argv )
{
    try {
	Py_Initialize();

	szarp::ipc ipc = szarp::ipc();

	py::object main_module(py::handle<>(py::borrowed(PyImport_AddModule("__main__"))));

	py::object main_namespace = main_module.attr("__dict__");
	main_namespace["IPC"] = py::class_<szarp::ipc>("IPC")
	    .def("getRead", &szarp::ipc::getRead)
	    .def("setRead", &szarp::ipc::setRead);
	main_namespace["ipc"] = py::ptr(&ipc);

	py::handle<> ignored(( PyRun_String( "ipc.setRead(123); print ipc.getRead()\n",
                                     Py_file_input,
                                     main_namespace.ptr(),
                                     main_namespace.ptr() ) ));
    } catch( py::error_already_set ) {
	PyErr_Print();
    }
}
