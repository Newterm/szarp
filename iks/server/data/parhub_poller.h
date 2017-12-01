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


struct TParamValue;

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
	ZmqSocketHolder(std::string url);
	bool recv(szarp::ParamsValues& pv) override;

};


class ParhubPoller {
	std::unique_ptr<SocketHolder> socket_holder;

	ParhubSubscriber& subscriber;

	std::future<void> poll_cv;
	std::atomic<bool> should_exit{false};

public:
	ParhubPoller(std::unique_ptr<SocketHolder> _socket_holder, ParhubSubscriber& _subscriber): socket_holder(std::move(_socket_holder)), subscriber(_subscriber) {
		run_poller();
	}

	ParhubPoller(std::string url, ParhubSubscriber& _subscriber): ParhubPoller(std::unique_ptr<SocketHolder>(new ZmqSocketHolder(url)), _subscriber) {}

	~ParhubPoller() {
		should_exit = true;
		if (poll_cv.valid()) poll_cv.wait();
	}

	void poll();

	void process_msg(szarp::ParamsValues& values);

private:
	void run_poller() {
		poll_cv = std::async(std::launch::async, [this](){ this->poll(); });
	}

};

struct TParamValue {
	// all integrals are kept in int64, all floating-points are kept in double
	mutable boost::variant<int64_t, double> value;

	template <typename T, typename std::enable_if<std::is_integral<T>::value,
	int>::type = 0> TParamValue(T v): value(int64_t(v)) {}

	template <typename T, typename std::enable_if<std::is_floating_point<T>::value,
	int>::type = 0> TParamValue(T v): value((double)v) {}

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


#endif
