#include "config.h"

#include <cppunit/extensions/HelperMacros.h>

#include "../iks/server/data/parhub_cache.h"
#include <queue>

class ParhubSubscriberStub: public ParhubSubscriber {
	std::function<void(size_t, TParamValue)> cb;

public:
	void param_value_changed(size_t param_ipc_ind, TParamValue value) override {
		cb(param_ipc_ind, value);
	}

	void set_callback(std::function<void(size_t, TParamValue)> new_cb) {
		cb = new_cb;
	}
};

class SocketStub: public SocketHolder {
public:
	std::queue<std::pair<size_t, TParamValue>> msgs_to_send;

	SocketStub(std::vector<std::pair<size_t, TParamValue>>& _to_send) {
		for (auto tp: _to_send) { msgs_to_send.push(tp); }
	}

	
	bool recv(szarp::ParamsValues& pvs) override {
		if (msgs_to_send.size() != 0) {
			auto msg = msgs_to_send.front();
			msgs_to_send.pop();

			pvs.clear_param_values();
			auto pv = pvs.add_param_values();

			pv->set_param_no(msg.first);
			if (msg.second.has<int64_t>()) {
				pv->set_int_value(msg.second.get<int64_t>());
			} else if (msg.second.has<double>()) {
				pv->set_double_value(msg.second.get<double>());
			}

			return true;
		}
		
		return false;
	}

};

class ParhubCacheTest: public CPPUNIT_NS::TestFixture {
	void test();

	CPPUNIT_TEST_SUITE( ParhubCacheTest );
	CPPUNIT_TEST( test );
	CPPUNIT_TEST_SUITE_END();
};


// workaround for non-moving lambda captures (delete in C++14)
template <typename T>
struct Mover {
	T t;
	Mover(T&& _t): t(std::forward<T>(_t)) {}

	Mover(const Mover& other): t(std::move(other.t)) {}
	Mover(Mover&& other): t(std::move(other.t)) {}
	Mover& operator()(const Mover& other) { t = std::move(other.t); return *this; }
	Mover& operator()(Mover&& other) { t = std::move(other.t); return *this; }

	T* operator->() { return &t; }
};

void ParhubCacheTest::test() {
	ParhubSubscriberStub subscriber;

	std::promise<void> promise;
	auto cv = promise.get_future();
	Mover<std::promise<void>> m(std::move(promise));

	size_t count = 10;
	std::vector<std::pair<size_t, TParamValue>> ret;

	subscriber.set_callback(
		[&m, &ret, &count] (size_t param_ipc_ind, TParamValue value) {
			ret.emplace_back(param_ipc_ind, TParamValue(value));
			if (--count == 0)
				m->set_value();
		}
	);

	std::vector<std::pair<size_t, TParamValue>> to_send = {
		{0, TParamValue(0.3)},
		{0, TParamValue(-0.9)},
		{1, TParamValue(3)},
		{2, TParamValue(5)},
		{13, TParamValue(5065)},
		{32, TParamValue(-4444)},
		{1, TParamValue(4)},
		{1, TParamValue(9)},
		{1, TParamValue(-10)},
		{2, TParamValue(10)}
	};

	auto socket = std::unique_ptr<SocketStub>(new SocketStub(to_send));
	ParhubListener listener(std::move(socket), subscriber);

	cv.wait();

	CPPUNIT_ASSERT(ret == to_send);
}

CPPUNIT_TEST_SUITE_REGISTRATION( ParhubCacheTest );
