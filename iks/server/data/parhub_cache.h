#ifndef __PARHUBCACHE__H__
#define __PARHUBCACHE__H__

#include <unordered_set>
#include <mutex>
#include <atomic>
#include <future>
#include <type_traits>
#include <boost/variant.hpp>
#include <zmq.hpp>

#include "protobuf/paramsvalues.pb.h"

#include <iostream>


struct TParamValue {
	mutable boost::variant<int64_t, double> value;

	template <typename T, typename std::enable_if<
		std::is_integral<T>::value, int
	>::type = 0>
	TParamValue(T v): value(int64_t(v)) {}

	template <typename T, typename std::enable_if<
		std::is_floating_point<T>::value, int
	>::type = 0>
	TParamValue(T v): value((double)v) {}

	TParamValue(const szarp::ParamValue& pv) {
		if (pv.has_int_value())
			value = int64_t(pv.int_value());
		if (pv.has_float_value())
			value = double(pv.float_value());
		if (pv.has_double_value())
			value = double(pv.double_value());
	}

	TParamValue(const TParamValue& o): value(o.value) {}
	TParamValue& operator=(const TParamValue& o) {
		value = o.value;
		return *this;
	}

	TParamValue(TParamValue&& o): value(std::move(o.value)) {}
	TParamValue& operator=(TParamValue&& o) {
		value = std::move(o.value);
		return *this;
	}


	template <typename T>
	T get() const { return boost::get<T>(value); }

	template <typename T>
	bool has() const {
		if (T* val = boost::get<T>(&value))
			return true;
		else
			return false;
	}

	bool operator==(const TParamValue& other) const {
		return value == other.value;
	}
};


class ParhubSubscriber {
public:
	virtual void param_value_changed(size_t param_ipc_ind, TParamValue value) = 0;
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
	ZmqSocketHolder(std::string url) {
		int zero = 0;
		socket.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));

		int recv_timeout = 100; // msecs
		socket.setsockopt(ZMQ_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

		socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
		socket.connect(url.c_str());
	}

	bool recv(szarp::ParamsValues& pv) override {
		zmq::message_t msg;
		if (socket.recv(&msg)) {
			if (pv.ParseFromArray(msg.data(), msg.size()))
				return true;
		}

		return false;
	}
};


class ParhubListener {
	std::unique_ptr<SocketHolder> socket_holder;

	std::mutex _in_mutex;
	std::mutex _params_mutex;

	ParhubSubscriber& subscriber;

	std::future<void> poll_cv;
	std::atomic<bool> should_exit{false};

public:
	ParhubListener(std::unique_ptr<SocketHolder> _socket_holder, ParhubSubscriber& _subscriber): socket_holder(std::move(_socket_holder)), subscriber(_subscriber) {
		run_poller();
	}

	ParhubListener(std::string url, ParhubSubscriber& _subscriber):
	    ParhubListener(std::unique_ptr<SocketHolder>(new ZmqSocketHolder(url)), _subscriber) {}

	~ParhubListener() {
		should_exit = true;
		if (poll_cv.valid()) poll_cv.wait();
	}

	void poll() {
		while (!should_exit) {
			szarp::ParamsValues values;
			if (socket_holder->recv(values)) {
				process_msg(values);
			}
		}
	}

	void process_msg(szarp::ParamsValues& values) {
		std::lock_guard<std::mutex> _guard(_params_mutex);
		for (int i = 0; i < values.param_values_size(); i++) {
			const szarp::ParamValue& param_value = values.param_values(i);

			auto param_no = param_value.param_no();

			subscriber.param_value_changed(param_no, param_value);
		}
	}

private:
	void run_poller() {
		poll_cv = std::async(std::launch::async, [this](){ this->poll(); });
	}

};


#endif
