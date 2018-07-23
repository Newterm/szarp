#include "parhub_poller.h"

ZmqSocketHolder::ZmqSocketHolder(std::string url) {
	int zero = 0;
	socket.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));

	int recv_timeout = 500; // msecs
	socket.setsockopt(ZMQ_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

	socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
	socket.connect(url.c_str());
}

bool ZmqSocketHolder::recv(szarp::ParamsValues& pv) {
	zmq::message_t msg;
	if (socket.recv(&msg)) {
		if (pv.ParseFromArray(msg.data(), msg.size()))
			return true;
	}

	return false;
}

ParhubPoller::ParhubPoller(std::unique_ptr<SocketHolder> _socket_holder, ParhubSubscriber& _subscriber): socket_holder(std::move(_socket_holder)), subscriber(_subscriber) {
	run_poller();
}

ParhubPoller::ParhubPoller(std::string url, ParhubSubscriber& _subscriber): ParhubPoller(std::unique_ptr<SocketHolder>(new ZmqSocketHolder(url)), _subscriber) {}

ParhubPoller::~ParhubPoller() {
	should_exit = true;
	if (poll_cv.valid()) poll_cv.wait();
}

void ParhubPoller::run_poller() {
	poll_cv = std::async(std::launch::async, [this](){ this->poll(); });
}

void ParhubPoller::poll() {
	// not the best way to do this
	// TODO: change to select()
	while (!should_exit) {
		szarp::ParamsValues values;
		if (socket_holder->recv(values)) {
			process_msg(values);
		}
	}
}

void ParhubPoller::process_msg(szarp::ParamsValues& values) {
	for (int i = 0; i < values.param_values_size(); i++) {
		const szarp::ParamValue& param_value = values.param_values(i);

		auto param_no = param_value.param_no();

		subscriber.param_value_changed(param_no, param_value);
	}
}
