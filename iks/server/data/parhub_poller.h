#ifndef __PARHUBPOLLER__H__
#define __PARHUBPOLLER__H__

#include <unordered_set>
#include <mutex>
#include <atomic>
#include <future>
#include <type_traits>
#include <boost/variant.hpp>
#include <zmq.hpp>

#include "protobuf/paramsvalues.pb.h"


class ParhubSubscriber {
public:
	virtual void param_value_changed(size_t param_ipc_ind, const szarp::ParamValue& value) = 0;
};

class SocketHolder {
public:
	virtual bool recv(szarp::ParamsValues& pv) = 0;
	virtual ~SocketHolder() {}
};

class ZmqSocketHolder: public SocketHolder {
	zmq::context_t ctx{1};
	zmq::socket_t socket{ctx, ZMQ_SUB};

public:
	ZmqSocketHolder(std::string url);
	bool recv(szarp::ParamsValues& pv) override;

};


class ParhubPoller {
	std::unique_ptr<SocketHolder> socket_holder;

	ParhubSubscriber& subscriber;

	std::future<void> poll_cv;
	std::atomic<bool> should_exit{false};

public:
	ParhubPoller(std::unique_ptr<SocketHolder> _socket_holder, ParhubSubscriber& _subscriber);
	ParhubPoller(std::string url, ParhubSubscriber& _subscriber);

	~ParhubPoller();

	void poll();

	void process_msg(szarp::ParamsValues& values);

private:
	void run_poller();

};

#endif
