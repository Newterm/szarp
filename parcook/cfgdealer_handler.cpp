#include "cfgdealer_handler.h"
#include "zmq.hpp"

namespace basedmn {

ConfigDealerHandler::DeviceInfo::DeviceInfo(const pt::ptree& conf, size_t _device_no) {
	device_no = _device_no;
	for (const auto& unit_conf: conf) {
		if (unit_conf.first != "unit") continue;

		units.push_back(UnitInfo(unit_conf.second));
		params_count += units.back().params_count;
		sends_count += units.back().sends_count;
	}

	attrs = conf.get_child("<xmlattr>");
	first_ipc_param_ind = attrs.get<size_t>("initial_param_no");
}


ConfigDealerHandler::ConfigDealerHandler(const ArgsManager& args_mgr) {
	/* libpar params */

	ipk_path = *args_mgr.get<std::string>("IPK");

	cfgdealer_address = *args_mgr.get<std::string>("cfgdealer-address");

	ipc_info = std::move(IPCInfo{
		*args_mgr.get<std::string>("parcook_path"),
		*args_mgr.get<std::string>("linex_cfg")
	});

	single = args_mgr.has("single");
	zmq::context_t context(1);
	zmq::socket_t socket(context, ZMQ_REQ);

	socket.connect ("tcp://localhost:5555");
	// cfgdealer_address
	
	auto d_no = *args_mgr.get<unsigned int>("device-no");
	auto d_str = std::to_string(d_no);
	zmq::message_t request (d_str.size());
	memcpy(request.data(), d_str.c_str(), d_str.size());
	socket.send (request);

	//  Get the reply.
	zmq::message_t reply;
	socket.recv (&reply);
	std::string&& conf_str{std::move((char*) reply.data()), reply.size()};
	std::cout << "Received " << conf_str << std::endl;

	conf_tree;
	std::istringstream ss(std::move(conf_str));
	pt::read_xml(ss, conf_tree);

	for (const auto& device_conf: conf_tree.get_child("params")) {
		if (device_conf.first != "device") continue;
		device = std::move(DeviceInfo(device_conf.second, d_no));
	}
}

} // namespace basedmn
