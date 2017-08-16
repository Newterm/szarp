#ifndef __CFGDEALER_HPP__
#define __CFGDEALER_HPP__

#include <zmq.hpp>
#include <boost/property_tree/ptree.hpp>
#include "argsmgr.h"

class CfgDealer {
public:
	CfgDealer();

private:
	void serve();
	void process_request(const zmq::message_t& request);

	const boost::property_tree::ptree get_device_config(const size_t device_no);
	void output_device_config(const boost::property_tree::tree& device_config);

	void configure_socket(const ArgsManager&);
	void prepare_configs(const ArgsManager&);
};

#endif
