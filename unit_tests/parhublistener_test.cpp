#include "config.h"

#include <cppunit/extensions/HelperMacros.h>

#include "../iks/server/data/parhub_poller.h"
#include <queue>

#include "sz4/util.h"

class ParhubSubscriberStub: public ParhubSubscriber {
	std::promise<void> pm;
	size_t count = 10;

public:
	std::vector<std::pair<size_t, double>> vps;

public:
	ParhubSubscriberStub(std::promise<void>&& _pm): pm(std::move(_pm)) {}

	void param_value_changed(size_t param_ipc_ind, const szarp::ParamValue& value) override {
		auto vp = sz4::cast_param_value<double>(value, 1);
		double val = vp.value;
		vps.emplace_back(param_ipc_ind, val);
		if (--count == 0)
			pm.set_value();
	}
};

class SocketStub: public SocketHolder {
public:
	std::queue<std::pair<size_t, double>> msgs_to_send;

	SocketStub(std::vector<std::pair<size_t, double>>& _to_send) {
		for (auto tp: _to_send) { msgs_to_send.push(tp); }
	}
	
	bool recv(szarp::ParamsValues& pvs) override {
		if (msgs_to_send.size() != 0) {
			auto msg = msgs_to_send.front();
			msgs_to_send.pop();

			pvs.clear_param_values();
			auto pv = pvs.add_param_values();
			
			pv->set_param_no(msg.first);
			pv->set_time(100);
			pv->set_is_nan(false);
			pv->set_double_value(msg.second);

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

void ParhubCacheTest::test() {
	std::promise<void> pm;
	auto cv = pm.get_future();

	ParhubSubscriberStub subscriber(std::move(pm));

	std::vector<std::pair<size_t, double>> to_send = {
		{0, 0.3},
		{0, -0.9},
		{1, 3},
		{2, 5},
		{13, 5065},
		{32, -4444},
		{1, 4},
		{1, 9},
		{1, -10},
		{2, 10}
	};

	auto socket = std::make_unique<SocketStub>(to_send);
	auto listener = new ParhubPoller(std::move(socket), subscriber);

	CPPUNIT_ASSERT(cv.valid());
	cv.wait();

	CPPUNIT_ASSERT(subscriber.vps == to_send);
	delete listener;
}

CPPUNIT_TEST_SUITE_REGISTRATION( ParhubCacheTest );
