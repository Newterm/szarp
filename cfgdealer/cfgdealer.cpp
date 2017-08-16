#include <string>
#include <iostream>
#include <unistd.h>

#include <boost/lexical_cast.hpp>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include <exception>

#include "cfgdealer.hpp"

namespace pt = boost::property_tree;

class CfgDealerArgs: public ArgsHolder {
public:
	po::options_description get_options() override {
		po::options_description desc{"Server configuration"};
		desc.add_options()
			("sub_addr,a", po::value<std::string>()->default_value(""), "Address to serve configurations on");

		return desc;
	}
};

CfgDealer::CfgDealer(const ArgsManager& args_mgr) {
	configure_socket(args_mgr);
	prepare_configs(args_mgr);
}

void CfgDealer::configure_socket(const ArgsManager& args_mgr) {
	sub_addr = *args_mgr.get<std::string>("sub_addr")
	socket.reset(new zmq::socket_t(context, ZMQ_REP));
	socket->bind(sub_addr);
}

void CfgDealer::prepare_configs(const ArgsManager& args_mgr) {
	pt::read_xml("/etc/szarp/default/config/params.xml", *tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments);

	no_devices = tree->get_child("params").count("device");

	size_t param_no = 1;
	size_t send_no = 1;
	size_t unit_no = 1;

	for (auto& device: tree->get_child("params")) {
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
}

void CfgDealer::serve() {
	while (true) {
		zmq::message_t request;

		//  Wait for next request from client
		socket.recv (&request);
		process_request(request);
	}
}

void CfgDealer::process_request(const zmq::message_t& request) {
	auto device_no = boost::lexical_cast<unsigned int>(std::string((char*) request.data(), request.size()));

	if (device_no > no_devices) {
		return;
	}

	const auto device_tree = get_device_config(device_no);

	output_device_config(device_config);
}

const pt::ptree CfgDealer::get_device_config(const size_t device_no) {
	pt::ptree params_tree;

	const auto attr_tree = tree.get_child("params.<xmlattr>");
	params_tree.add_child("params.<xmlattr>", attr_tree);
	auto dev = tree.get_child("params").begin();
	// device_no starts from 1, but there is an <xmlattr> node
	std::advance(dev, device_no);

	params_tree.add_child("params.device", dev->second);
	
	return params_tree;
}

void CfgDealer::output_device_config(const pt::tree& device_config) {
	std::ostringstream oss;
	pt::xml_parser::write_xml(oss, device_config);

	std::string params_str = oss.str();

	//  Send reply back to client
	zmq::message_t reply (params_str.size());
	memcpy (reply.data(), params_str.c_str(), params_str.size());
	socket.send (reply);
}

int main (int argc, char ** argv) {
	args_mgr = ArgsManager("cfgdealer");
	args_mgr.parse(argc, argv, {new DefaultArgs(), new CfgDealerArgs()});

	CfgDealer cfg_dealer(args_mgr);
	cfg_dealer->serve();

    return 0;
}
