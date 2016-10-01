#include <zmq.hpp>
#include <poll.h>

#include "config.h"

#include "szarp_config.h"
#include "zmqhandler.h"
#include "protobuf/paramsvalues.pb.h"

#include <cppunit/extensions/HelperMacros.h>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>


class ZmqHandlerTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( ZmqHandlerTest );
	CPPUNIT_TEST( test );
	CPPUNIT_TEST_SUITE_END();

	TSzarpConfig config;

	std::unique_ptr<zmq::socket_t> sock;
	std::unique_ptr<zmq::socket_t> sock2;

	std::string sub_uri = "inproc://aa";
	std::string pub_uri = "inproc://bb";

	std::unique_ptr<zmq::context_t> context;
public:
	void test();
	void setUp();
	void tearDown();
};

namespace {
void delete_str(void *, void *buf) {
	delete (std::string*) buf;
}
}

void ZmqHandlerTest::setUp() {
	config.AddDevice(config.createDevice(L"/bin/true", L"fake"));
	TRadio * pr = config.GetFirstDevice()->AddRadio(config.createRadio(config.GetFirstDevice()));
	TUnit * pu = pr->AddUnit(config.createUnit(pr, 'x'));

	auto param1 = new TParam(pu, &config);
	param1->SetConfigId(0);
	param1->SetName(L"a:b:a");
	param1->SetParamId(0);
	pu->AddParam(param1);

	auto param2 = new TParam(NULL, &config);
	param2->SetConfigId(0);
	param2->SetParamId(1);
	param2->SetName(L"a:b:c");
	config.AddDefined(param2);

	auto sendparam = new TSendParam(pu);
	sendparam->Configure(L"a:b:c", 1, 1, PROBE, 1);
	pu->AddParam(sendparam);

	context.reset(new zmq::context_t(1));


	sock.reset(new zmq::socket_t(*context, ZMQ_PUB));
	sock->bind(sub_uri.c_str());

	sock2.reset(new zmq::socket_t(*context, ZMQ_SUB));
	sock2->bind(pub_uri.c_str());

	int zero = 0;
	sock->setsockopt(ZMQ_LINGER, &zero, sizeof(zero));
	sock2->setsockopt(ZMQ_LINGER, &zero, sizeof(zero));
}

void ZmqHandlerTest::tearDown() {
	sock->close();
	sock2->close();
}

void ZmqHandlerTest::test() {
	zmqhandler handler(&config, config.GetFirstDevice(), *context, sub_uri, pub_uri);
	///XXX (BUG IN ZMQ?!?): for inproc uncoditional receive is needed in order to subsequent
	// notifications to work
	handler.receive();

	int subsocket = handler.subsocket();
	CPPUNIT_ASSERT(subsocket != -1);

	szarp::ParamsValues values;
	auto value = values.add_param_values();
	value->set_param_no(1);
	value->set_time(1);
	value->set_int_value(1);
	std::string* buffer = new std::string();
	google::protobuf::io::StringOutputStream stream(buffer);
	values.SerializeToZeroCopyStream(&stream);

	zmq::message_t msg((void*)buffer->data(), buffer->size(),
		delete_str, buffer);
	sock->send(msg);

	struct pollfd fd;
	fd.fd = subsocket;
	fd.events = POLLIN;

	int r = poll(&fd, 1, 3000);
	CPPUNIT_ASSERT_EQUAL(1, r);
}

CPPUNIT_TEST_SUITE_REGISTRATION( ZmqHandlerTest );
