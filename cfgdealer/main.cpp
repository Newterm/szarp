#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>

#include <boost/lexical_cast.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include <exception>

namespace pt = boost::property_tree;

int main (int argc, char ** argv) {
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5555");

	pt::ptree tree;
	pt::read_xml("/etc/szarp/default/config/params.xml", tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments);

	const pt::ptree &attr_tree = tree.get_child("params.<xmlattr>");

	const size_t no_devices = tree.get_child("params").count("device");
	std::cout << no_devices << std::endl;


	size_t param_no = 1;
	size_t send_no = 1;
	size_t unit_no = 1;
	for (auto& device: tree.get_child("params")) {
		if (device.first != "device") continue;

		device.second.put("<xmlattr>.initial_param_no", param_no);
		device.second.put("<xmlattr>.initial_send_no", send_no);

		for (auto& unit: device.second) {
			if (unit.first != "unit") continue;

			unit.second.put("<xmlattr>.unit_no", unit_no++);

			param_no += unit.second.count("param");
			send_no += unit.second.count("send");
		}
	}

    while (true) {
        zmq::message_t request;

        //  Wait for next request from client
        socket.recv (&request);

		auto device_no = boost::lexical_cast<unsigned int>(std::string((char*) request.data(), request.size()));
        std::cout << "Received " << device_no << std::endl;

		pt::ptree params_tree;

		if (device_no <= no_devices) {
			params_tree.add_child("params.<xmlattr>", attr_tree);
			auto dev = tree.get_child("params").begin();
			// device_no starts from 1, but there is an <xmlattr> node
			std::advance(dev, device_no);

			params_tree.add_child("params.device", dev->second);
		}

		std::ostringstream oss;
		pt::xml_parser::write_xml(oss, params_tree);

		std::string params_str = oss.str();
		std::cout << "Sending " << params_str << std::endl;

        //  Send reply back to client
        zmq::message_t reply (params_str.size());
        memcpy (reply.data(), params_str.c_str(), params_str.size());
        socket.send (reply);
    }
    return 0;
}
