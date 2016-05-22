#include <zmq.hpp>

#include "config.h"

#include <condition_variable>

#include "protobuf/paramsvalues.pb.h"

#include "szarp_config.h"

#include "sz4/defs.h"
#include "sz4/block.h"
#include "sz4/block.h"
#include "sz4/factory.h"
#include "sz4/live_observer.h"
#include "sz4/live_cache.h"
#include "sz4/live_cache_templ.h"

#include <cppunit/extensions/HelperMacros.h>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

class Sz4LiveCache : public CPPUNIT_NS::TestFixture
{
	void test();

	CPPUNIT_TEST_SUITE( Sz4LiveCache );
	CPPUNIT_TEST( test );
	CPPUNIT_TEST_SUITE_END();
};

namespace {
void delete_str(void *, void *buf) {
	delete (std::string*) buf;
}

class observer : public sz4::live_values_observer {
	std::mutex m_mutex;
	std::condition_variable m_cond;
	bool m_got_value = false;
public:
	bool wait() {
		std::unique_lock<std::mutex> guard(m_mutex);
		if (m_got_value)
			return true;

		m_cond.wait_until(guard,
			std::chrono::system_clock::now() + 
				std::chrono::milliseconds(10));

		return m_got_value;
	}

	void new_live_value(szarp::ParamValue *value) override {
		std::lock_guard<std::mutex> guard(m_mutex);
		m_got_value = true;
		m_cond.notify_one();
	}

	void set_live_block(sz4::generic_live_block* block) override {

	}

};

}


void Sz4LiveCache::test() {
	observer obs;

	std::string uri("inproc://blah");
	auto context = new zmq::context_t(1);

	TSzarpConfig config;
	config.SetConfigId(0);

	TParam* param = new TParam(NULL, &config);
	param->SetConfigId(0);
	param->SetParamId(0);
	config.AddDefined(param);

	sz4::live_cache_config live_config{
			{ { uri, &config } },
			1000
		};
	sz4::live_cache cache(live_config, context);

	cache.register_cache_observer(param, &obs);

	{
		zmq::socket_t sock(*context, ZMQ_PUB);
		sock.bind(uri.c_str());

		int zero = 0;
		sock.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));

		szarp::ParamsValues values;
		auto value = values.add_param_values();
		value->set_param_no(0);
		value->set_time(1);
		value->set_int_value(1);

		do {
			std::string* buffer = new std::string();
			google::protobuf::io::StringOutputStream stream(buffer);
			values.SerializeToZeroCopyStream(&stream);

			zmq::message_t msg(
				(void*)buffer->data(),
				buffer->size(),
				delete_str,
				buffer);

			sock.send(msg);
		} while (!obs.wait());

	}
}

CPPUNIT_TEST_SUITE_REGISTRATION( Sz4LiveCache );
