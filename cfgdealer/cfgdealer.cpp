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
	prepare_configs(args_mgr);
	configure_socket(args_mgr);
}

void CfgDealer::configure_socket(const ArgsManager& args_mgr) {
	const auto sub_addr = *args_mgr.get<std::string>("cfgdealer-address");
	socket.reset(new zmq::socket_t(context, ZMQ_REP));
	socket->bind(sub_addr.c_str());
}

void CfgDealer::prepare_configs(const ArgsManager& args_mgr) {
	auto base = args_mgr.get<std::string>("base");
	if (base) {
		pt::read_xml(std::string("/opt/szarp/")+*base+std::string("/config/params.xml"), tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments, std::locale("pl_PL.UTF-8"));
	} else {
		pt::read_xml(std::string("/etc/szarp/default/config/params.xml"), tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments, std::locale("pl_PL.UTF-8"));
	}

	no_devices = tree.get_child(L"params").count(L"device");

	size_t param_no = 1;
	size_t send_no = 1;
	size_t unit_no = 1;

	std::unordered_set<std::wstring> sends_to_add;

	for (auto& device: tree.get_child(L"params")) {
		if (device.first != L"device") continue;

		device.second.put(L"<xmlattr>.initial_param_no", param_no);

		for (auto& unit: device.second) {
			if (unit.first != L"unit") continue;

			unit.second.put(L"<xmlattr>.unit_no", unit_no++);

			param_no += unit.second.count(L"param");
			send_no += unit.second.count(L"send");

			for (auto& send_param: unit.second) {
				if (send_param.first != L"send") continue;
				try {
					sends_to_add.insert(send_param.second.get<std::wstring>(L"<xmlattr>.param"));
				} catch (...) {
					// send without param (e.g. value)
				}
			}
		}
	}

	size_t param_ind = 0;
	for (auto& device: tree.get_child(L"params")) {
		if (device.first != L"device") continue;
		for (auto& unit: device.second) {
			if (unit.first != L"unit") continue;
			for (auto& param: unit.second) {
				if (param.first != L"param") continue;
				const auto& name = param.second.get<std::wstring>(L"<xmlattr>.name");
				if (sends_to_add.find(name) != sends_to_add.end()) {
					sends[name] = {param.second, param_ind};
				}

				++param_ind;
			}
		}
	}
}

void CfgDealer::serve() {
	while (true) {
		zmq::message_t request;

		//  Wait for next request from client
		socket->recv (&request);
		process_request(request);
	}
}

void CfgDealer::process_request(zmq::message_t& request) {
	auto device_no = boost::lexical_cast<unsigned int>(std::string((char*) request.data(), request.size()));

	if (device_no > no_devices) {
		szlog::log() << szlog::CRITICAL << "Invalid device number " << device_no << " requested, only " << no_devices << " devices in config. Did params.xml change?" << szlog::endl;
		return;
	}

	const auto device_config = get_device_config(device_no);

	output_device_config(device_config);
}

const pt::wptree CfgDealer::get_device_config(const size_t device_no) {
	pt::wptree params_tree;

	const auto attr_tree = tree.get_child(L"params.<xmlattr>");
	params_tree.add_child(L"params.<xmlattr>", attr_tree);
	auto dev = tree.get_child(L"params").begin();

	// device_no starts from 1, but we are skipping <xmlattr> node (it's always the first node in the tree)
	std::advance(dev, device_no);

	params_tree.add_child(L"params.device", dev->second);

	pt::wptree sends_tree;

	for (auto& unit: dev->second) {
		if (unit.first != L"unit") continue;
		for (auto& send_param: unit.second) {
			if (send_param.first != L"send") continue;
			const auto& name = send_param.second.get<std::wstring>(L"<xmlattr>.param");
			auto send_pair = sends[name];
			auto send_param_tree = send_pair.first;
			send_param_tree.put(L"<xmlattr>.ipc_ind", send_pair.second);
			sends_tree.add_child(L"param", send_param_tree);
		}
	}

	params_tree.add_child(L"params.sends", sends_tree);
	
	return params_tree;
}

void CfgDealer::output_device_config(const pt::wptree& device_config) {
	std::wostringstream oss;
	pt::xml_parser::write_xml(oss, device_config);

	std::string params_str = SC::S2A(oss.str());

	//  Send reply back to client
	zmq::message_t reply (params_str.size());
	memcpy (reply.data(), params_str.c_str(), params_str.size());
	socket->send (reply);
}

int main (int argc, char ** argv) {
	auto args_mgr = ArgsManager("cfgdealer");
	args_mgr.parse(argc, argv, DefaultArgs(), CfgDealerArgs());
	args_mgr.initLibpar();

	CfgDealer cfg_dealer(args_mgr);
	cfg_dealer.serve();

    return 0;
}
