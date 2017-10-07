#ifndef __CFGDEALER_HPP__
#define __CFGDEALER_HPP__

#include <zmq.hpp>
#include <boost/property_tree/ptree.hpp>
#include <unordered_map>
#include <unordered_set>
#include "argsmgr.h"

class CfgDealer {
public:
	CfgDealer(const ArgsManager& args_mgr);
	void serve();

	const boost::property_tree::wptree get_device_config(const size_t device_no);

private:
	void process_request(zmq::message_t& request);

	void output_device_config(const boost::property_tree::wptree& device_config);

	void configure_socket(const ArgsManager&);
	void prepare_configs(const ArgsManager&);

private:
	const size_t NO_PROCESSING_THREADS = 1;
	zmq::context_t context{NO_PROCESSING_THREADS};
	std::unique_ptr<zmq::socket_t> socket;
	boost::property_tree::wptree tree;

	std::unordered_map<std::wstring, std::pair<boost::property_tree::wptree, size_t>> sends;
	size_t no_devices;
};

#endif
