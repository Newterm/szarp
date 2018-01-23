#include <string>
#include <iostream>
#include <unistd.h>

#include "conversion.h"

#include "cfgdealer.hpp"
#include "argsmgr.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

class CfgDealerClientArgs: public ArgsHolder {
public:
	po::options_description get_options() const override {
		po::options_description desc{"Client configuration"};
		desc.add_options()
			("help,h", "Print this help messages")
			("cfgdealer-address,a", po::value<std::string>()->default_value("tcp://127.0.0.1:5555"), "Address to serve configurations on")
			("device-no,d", po::value<long>()->default_value(1), "Device number to ask for");

		return desc;
	}

	void parse(const po::parsed_options& parsed_opts, const po::variables_map& vm) const override {
		if (vm.count("help")) { 
			std::cout << *parsed_opts.description << std::endl;
			exit(0);
		} 
	}
};

int main (int argc, char ** argv) {
	auto args_mgr = ArgsManager("cfgdealer");
	args_mgr.parse(argc, argv, CfgDealerClientArgs());

	zmq::context_t context(1);
	zmq::socket_t socket(context, ZMQ_REQ);

	constexpr int timeout_ms = 1000;
	socket.setsockopt(ZMQ_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));

	auto cfgdealer_address = *args_mgr.get<std::string>("cfgdealer-address");
	socket.connect(cfgdealer_address.c_str());
	
	long device_no = *args_mgr.get<long>("device-no");
	auto d_str = std::to_string(device_no);

	zmq::message_t request (d_str.size());
	memcpy(request.data(), d_str.c_str(), d_str.size());

	socket.send(request);

	//  Get the reply.
	zmq::message_t reply;
	socket.recv (&reply);

	std::string conf_str{std::move((char*) reply.data()), reply.size()};
	if (conf_str.empty()) {
		std::cout << "Got no configuration from cfgdealer (is it running?)." << std::endl;
		return 1;
	}

	std::wistringstream ss(SC::L2S(std::move(conf_str)));

	pt::wptree conf_tree;
	pt::read_xml(ss, conf_tree);

#if BOOST_VERSION >= 106200
	pt::write_xml(std::wcout, conf_tree, pt::xml_parser::xml_writer_make_settings<std::wstring>(L' ', 2));
#else
	pt::write_xml(std::wcout, conf_tree, pt::xml_parser::xml_writer_make_settings<wchar_t>(L' ', 2));
#endif

	return 0;
}
