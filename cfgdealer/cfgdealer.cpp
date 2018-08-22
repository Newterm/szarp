#include <string>
#include <iostream>
#include <unistd.h>

#include <boost/lexical_cast.hpp>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include <exception>
#include "conversion.h"

#include "cfgdealer.hpp"
#include "liblog.h"

namespace pt = boost::property_tree;

class CfgDealerArgs: public ArgsHolder {
public:
	po::options_description get_options() const override {
		po::options_description desc{"Server configuration"};
		desc.add_options()
			("cfgdealer-address,a", po::value<std::string>()->default_value("tcp://*:5555"), "Address to serve configurations on");

		return desc;
	}
};

CfgDealer::CfgDealer(const ArgsManager& args_mgr) {
	prepare_config(args_mgr);
	configure_socket(args_mgr);
}

void CfgDealer::configure_socket(const ArgsManager& args_mgr) {
	const auto sub_addr = *args_mgr.get<std::string>("cfgdealer-address");

	try {
		socket.reset(new zmq::socket_t(context, ZMQ_REP));
		socket->bind(sub_addr.c_str());
	} catch(const zmq::error_t& e) {
		/* make the error as verbose as possible - this is critical */
		sz_log(0, "Error: could not bind to %s: %s, is another instance running? Cfgdealer will now exit.", sub_addr.c_str(), e.what());
		std::cout << "Encountered error, check previous logs" << std::endl;
		exit(1);
	} catch(const std::exception& e) {
		/* make the error as verbose as possible - this is critical */
		sz_log(0, "Could not initialize cfgdealer, error was %s", e.what());
		std::cout << "Encountered error, check previous logs" << std::endl;
		exit(1);
	}
}

void CfgDealer::prepare_config(const ArgsManager& args_mgr) {
	auto base = args_mgr.get<std::string>("base");
	if (base) {
		pt::read_xml(std::string("/opt/szarp/")+*base+std::string("/config/params.xml"), tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments, std::locale(""));
	} else {
		pt::read_xml(std::string("/etc/szarp/default/config/params.xml"), tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments, std::locale(""));
	}

	no_devices = tree.get_child("params").count("device");

	prepare_units();
	prepare_params();
}

void remove_nodes_not_matching(CfgDealer::ptree& tree, const std::vector<std::string>& nodes) {
	for (auto& node: tree) {
		if (std::find(nodes.cbegin(), nodes.cend(), node.first) == nodes.cend()) {
			tree.erase(node.first);
		}
	}
}

void CfgDealer::prepare_units() {
	size_t param_no = 1;
	size_t send_no = 1;
	size_t unit_no = 1;

	auto& params_tree = tree.get_child("params");
	remove_nodes_not_matching(params_tree, {"<xmlattr>", "device", "defined"}); // remove drawdefinable

	for (auto& device: params_tree) {
		if (device.first != "device") continue;
		remove_nodes_not_matching(device.second, {"<xmlattr>", "unit"});
		device.second.put("<xmlattr>.initial_param_no", param_no);

		for (auto& unit: device.second) {
			if (unit.first != "unit") continue;
			remove_nodes_not_matching(unit.second, {"<xmlattr>", "param", "send"});

			unit.second.put("<xmlattr>.unit_no", unit_no++);

			param_no += unit.second.count("param");
			send_no += unit.second.count("send");

			for (auto& param: unit.second) {
				if (param.first == "send") {
					try {
						sends_to_add.insert(param.second.get<std::string>("<xmlattr>.param"));
					} catch (...) {
						// send without param (e.g. value)
					}
				} else if (param.first == "param") {
					// remove junk nodes (raport, draw etc)
					remove_nodes_not_matching(param.second, {"<xmlattr>", "define"});
				}
			}
		}
	}
}

void CfgDealer::prepare_send_from_param(const std::pair<std::string, ptree>& send_pair, size_t& param_ind) {
	if (send_pair.first != "param") return;
	const auto& name = send_pair.second.get<std::string>("<xmlattr>.name");
	if (sends_to_add.find(name) != sends_to_add.end()) {
		sends[name] = {send_pair.second, param_ind};
	}

	++param_ind;
}

void CfgDealer::prepare_params() {
	size_t param_ind = 0;
	for (const auto& device: tree.get_child("params")) {
		if (device.first == "device") {
			for (const auto& unit: device.second) {
				if (unit.first != "unit") continue;
				for (auto& param: unit.second) {
					prepare_send_from_param(param, param_ind);
				}
			}
		} else if (device.first == "defined") {
			for (const auto& param: device.second) {
				prepare_send_from_param(param, param_ind);
			}
		}
	}
}

void CfgDealer::serve() {
	while (true) {
		zmq::message_t request;
		try {
			//  Wait for next request from client
			socket->recv (&request);
			process_request(request);
		} catch (const std::exception& e) {
			// we have to reply (otherwise zmq won't let us process messages)
			send_reply(e.what());
		}
	}
}

void CfgDealer::process_request(zmq::message_t& request) {
	auto device_no = boost::lexical_cast<unsigned int>(std::string((char*) request.data(), request.size()));

	if (device_no > no_devices) {
		szlog::log() << szlog::error << "Invalid device number " << device_no << " requested, only " << no_devices << " devices in config. Did params.xml change?" << szlog::endl;
		throw std::runtime_error("");
	}

	const auto device_config = get_device_config(device_no);

	output_device_config(device_config);
}

const CfgDealer::ptree CfgDealer::get_device_config(const size_t device_no) const {
	ptree params_tree;

	const auto attr_tree = tree.get_child("params.<xmlattr>");
	params_tree.add_child("params.<xmlattr>", attr_tree);
	auto dev = tree.get_child("params").begin();

	// device_no starts from 1, but we are skipping <xmlattr> node (it's always the first node in the tree)
	std::advance(dev, device_no);

	ptree sends_tree;

	for (auto& unit: dev->second) {
		if (unit.first != "unit") continue;
		for (auto& send_param: unit.second) {
			if (send_param.first != "send") continue;
			if (!send_param.second.count("param")) continue;
			const auto& name = send_param.second.get<std::string>("<xmlattr>.param");
			auto send_pair = sends.at(name);
			auto send_param_tree = std::move(send_pair.first);
			send_param_tree.put("<xmlattr>.ipc_ind", std::move(send_pair.second));
			sends_tree.add_child("param", std::move(send_param_tree));
		}
	}

	params_tree.add_child("params.sends", sends_tree);
	params_tree.add_child("params.device", dev->second);
	
	return params_tree;
}

void CfgDealer::output_device_config(const ptree& device_config) {
	std::ostringstream oss;
	pt::xml_parser::write_xml(oss, device_config);

	send_reply(oss.str());
}

void CfgDealer::send_reply(const std::string& str) {
	zmq::message_t reply(str.size());
	memcpy(reply.data(), str.c_str(), str.size());
	socket->send(reply);
}

int main (int argc, char ** argv) {
	auto args_mgr = ArgsManager("cfgdealer");
	args_mgr.parse(argc, argv, DefaultArgs(), CfgDealerArgs());
	args_mgr.initLibpar();

	szlog::init(args_mgr, "cfgdealer");

	CfgDealer cfg_dealer(args_mgr);
	cfg_dealer.serve();

    return 0;
}
