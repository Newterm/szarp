#include <zmq.hpp>
#include <memory>

#include "config.h"

#include <condition_variable>

#include "protobuf/paramsvalues.pb.h"

#include "szarp_config.h"

#include "sz4/block.h"
#include "sz4/buffer.h"
#include "sz4/factory.h"
#include "sz4/live_cache.h"
#include "sz4/block_cache.h"
#include "sz4/live_observer.h"
#include "sz4/live_cache_templ.h"
#include "sz4/real_param_entry.h"
#include "sz4/real_param_entry_templ.h"
#include "sz4/defs.h"

#include <cppunit/extensions/HelperMacros.h>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

class Sz4LiveCache : public CPPUNIT_NS::TestFixture
{
	void zmqHandlingTest ();
	void retentionTest ();
	void blockTest ();

	TSzarpConfig config;
	TParam* param;
	std::unique_ptr<zmq::socket_t> sock;
	std::unique_ptr<sz4::live_cache> cache;

	CPPUNIT_TEST_SUITE( Sz4LiveCache );
	CPPUNIT_TEST( zmqHandlingTest );
	CPPUNIT_TEST( retentionTest );
	CPPUNIT_TEST( blockTest );
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
	void tearDown();
};

void Sz4LiveCache::setUp() {

	param = new TParam(NULL, &config);
	param->SetConfigId(0);
	param->SetParamId(0);

	config.SetConfigId(0);
	config.AddDefined(param);

	std::string uri("inproc://blah");
	auto context = new zmq::context_t(1);

	sock.reset(new zmq::socket_t(*context, ZMQ_PUB));
	sock->bind(uri.c_str());
	int zero = 0;
	sock->setsockopt(ZMQ_LINGER, &zero, sizeof(zero));

	sz4::live_cache_config live_config{
			{ { uri, &config } },
			1000
		};

	cache.reset(new sz4::live_cache(live_config, context));
}

void Sz4LiveCache::tearDown() {
	sock.reset();
}

namespace {
void delete_str(void *, void *buf) {
	delete (std::string*) buf;
}

class observer1 : public sz4::live_values_observer {
	std::mutex m_mutex;
	std::condition_variable m_cond;
	bool m_got_value = false;
public:
	bool wait() {
		std::unique_lock<std::mutex> guard(m_mutex);
		if (m_got_value)
			return true;

		m_cond.wait(guard);
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

void Sz4LiveCache::zmqHandlingTest() {
	observer1 obs;

	szarp::ParamsValues values;
	auto value = values.add_param_values();
	value->set_param_no(0);
	value->set_time(1);
	value->set_int_value(1);

	cache->register_cache_observer(param, &obs);

	std::string* buffer = new std::string();
	google::protobuf::io::StringOutputStream stream(buffer);
	values.SerializeToZeroCopyStream(&stream);

	zmq::message_t msg((void*)buffer->data(), buffer->size(),
		delete_str, buffer);

	sock->send(msg);

	obs.wait();

}

void Sz4LiveCache::retentionTest() {
	sz4::live_block<short, sz4::second_time_t> block(1000);
	block.process_live_value(1, 1);

	sz4::second_time_t t;

	block.get_first_time(t);
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(1), t);

	block.get_last_time(t);
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(1), t);

	block.process_live_value(500, 2);

	block.get_first_time(t);
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(1), t);

	block.get_last_time(t);
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(500), t);

	block.process_live_value(2000, 3);

	block.get_first_time(t);
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(500), t);

	block.get_last_time(t);
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(2000), t);

	block.process_live_value(4000, 3);

	block.get_first_time(t);
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(500), t);

	block.get_last_time(t);
	CPPUNIT_ASSERT_EQUAL(sz4::second_time_t(4000), t);
}

namespace {

class fake_base {
	sz4::block_cache m_cache;
public:
	sz4::block_cache* cache() { return &m_cache; }
};

class fake_block_entry : public sz4::file_block_entry<short, sz4::second_time_t, fake_base> {
public:
	typedef sz4::file_block_entry<short,sz4::second_time_t, fake_base> parent;
	fake_block_entry(const sz4::second_time_t& start, sz4::block_cache* cache)
			: parent(start, std::wstring(), cache) {

		this->m_block = new typename parent::block_type(this->m_start_time, this, this->m_cache);
	}

	parent::block_type* block() {
		return this->m_block;
	}

	void refresh_if_needed() {}
};

class real_entry : public sz4::real_param_entry_in_buffer<short, sz4::second_time_t, fake_base> {
public:
	real_entry(fake_base *base, TParam* param) : sz4::real_param_entry_in_buffer<short, sz4::second_time_t, fake_base>(base, param, std::wstring())
	{
		m_refresh_file_list = false;	
	}

	map_type& entry_blocks() {
		return m_blocks;
	}

};

class no_data_search_condition : public sz4::search_condition {
public:
	bool operator()(const short& v) const {
		return v != std::numeric_limits<short>::min();
	}

	bool operator()(const int& v) const {
		return v != std::numeric_limits<int>::min();

	}

	bool operator()(const float& v) const {
#ifndef MINGW32
		return !isnanf(v);
#else
		return !std::isnan(v);
#endif
	}

	bool operator()(const double& v) const {
		return !std::isnan(v);
	}
};
}

void Sz4LiveCache::blockTest() {
	sz4::live_block<short, sz4::second_time_t> live_block(1000);
	fake_base base;

	real_entry entry(&base, param);
	entry.set_live_block(&live_block);

	typedef sz4::weighted_sum<short, sz4::second_time_t> sum_t;
	typedef sz4::value_sum<short>::type sum_v;
	typedef sz4::second_time_t sec_t;
	typedef sz4::time_difference<sec_t>::type dif_t;

	sum_t sum;
	sum_t::time_diff_type weight;

	auto block = new fake_block_entry(sec_t(1000), base.cache());
	entry.entry_blocks().insert(
				std::pair<sec_t, fake_block_entry::parent*>(1000, block));

	std::vector<sz4::value_time_pair<short, sz4::second_time_t> > values{ { 5, 1500 } };
	block->block()->set_data(values);

	entry.get_weighted_sum_impl(sec_t(1000), sec_t(3000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sum_v(5 * 500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(dif_t(500), weight);
	CPPUNIT_ASSERT_EQUAL(dif_t(1500), sum.no_data_weight());
	CPPUNIT_ASSERT(!sum.fixed());

	live_block.process_live_value(2000, 10);

	CPPUNIT_ASSERT_EQUAL(sec_t(1499), entry.search_data_left_impl(2000, 1000, PT_SEC10, no_data_search_condition()));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sec_t>::invalid_value, entry.search_data_right_impl(1500, 3000, PT_SEC10, no_data_search_condition()));

	live_block.process_live_value(3000, 10);

	sum = sum_t();

	entry.get_weighted_sum_impl(sec_t(1000), sec_t(3000), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sum_v(10 * 1000 + 5 * 500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(dif_t(1500), weight);
	CPPUNIT_ASSERT_EQUAL(dif_t(500), sum.no_data_weight());

	CPPUNIT_ASSERT(sum.fixed());

	sum = sum_t();

	entry.get_weighted_sum_impl(sec_t(1000), sec_t(3500), PT_SEC10, sum);
	CPPUNIT_ASSERT_EQUAL(sum_v(10 * 1000 + 5 * 500), sum.sum(weight));
	CPPUNIT_ASSERT_EQUAL(dif_t(1500), weight);
	CPPUNIT_ASSERT_EQUAL(dif_t(1000), sum.no_data_weight());

	CPPUNIT_ASSERT(!sum.fixed());

	auto search_cond = no_data_search_condition();

	CPPUNIT_ASSERT_EQUAL(sec_t(2500), entry.search_data_left_impl(2500, 1000, PT_SEC10, search_cond));
	CPPUNIT_ASSERT_EQUAL(sec_t(2999), entry.search_data_left_impl(3500, 1000, PT_SEC10, search_cond));
	CPPUNIT_ASSERT_EQUAL(sec_t(1499), entry.search_data_left_impl(1999, 1000, PT_SEC10, search_cond));

	CPPUNIT_ASSERT_EQUAL(sec_t(1300), entry.search_data_right_impl(1300, 2000, PT_SEC10, search_cond));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sec_t>::invalid_value, entry.search_data_right_impl(1500, 2000, PT_SEC10, search_cond));
	CPPUNIT_ASSERT_EQUAL(sec_t(2000), entry.search_data_right_impl(1500, 3000, PT_SEC10, search_cond));
	CPPUNIT_ASSERT_EQUAL(sec_t(2100), entry.search_data_right_impl(2100, 3000, PT_SEC10, search_cond));
	CPPUNIT_ASSERT_EQUAL(sz4::time_trait<sec_t>::invalid_value, entry.search_data_right_impl(3000, 5000, PT_SEC10, search_cond));

	sum = sum_t();

	entry.get_weighted_sum_impl(sec_t(3000), sec_t(3500), PT_SEC10, sum);
	sum.sum(weight);
	CPPUNIT_ASSERT_EQUAL(dif_t(0), weight);
	CPPUNIT_ASSERT_EQUAL(dif_t(500), sum.no_data_weight());

	CPPUNIT_ASSERT(!sum.fixed());

}

CPPUNIT_TEST_SUITE_REGISTRATION( Sz4LiveCache );
