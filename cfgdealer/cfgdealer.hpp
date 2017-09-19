#ifndef __CFGDEALER_HPP__
#define __CFGDEALER_HPP__

#include <zmq.hpp>
#include <boost/property_tree/ptree.hpp>
#include "argsmgr.h"

class CfgDealer {
public:
	CfgDealer(const ArgsManager& args_mgr);
	void serve();

private:
	void process_request(zmq::message_t& request);

	const boost::property_tree::ptree get_device_config(const size_t device_no);
	void output_device_config(const boost::property_tree::ptree& device_config);

	void configure_socket(const ArgsManager&);
	void prepare_configs(const ArgsManager&);

private:
	std::unique_ptr<zmq::socket_t> socket;
	std::unique_ptr<boost::property_tree::ptree> tree;
	size_t no_devices;
};

#endif
