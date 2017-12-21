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

	using wptree = boost::property_tree::wptree;
	const wptree get_device_config(const size_t device_no);

private:
	void process_request(zmq::message_t& request);
	void send_reply(const std::string& str);

	void output_device_config(const wptree& device_config);

	void configure_socket(const ArgsManager&);

	void prepare_config(const ArgsManager&);

	void prepare_units();
	void prepare_params();
	void prepare_send_from_param(const std::pair<std::wstring, wptree>& send_pair, size_t& param_ind);

private:
	const int NO_PROCESSING_THREADS = 1;
	zmq::context_t context{NO_PROCESSING_THREADS};
	std::unique_ptr<zmq::socket_t> socket;
	wptree tree;

	std::unordered_set<std::wstring> sends_to_add;

	std::unordered_map<std::wstring, std::pair<wptree, size_t>> sends;
	size_t no_devices;

};

#endif
