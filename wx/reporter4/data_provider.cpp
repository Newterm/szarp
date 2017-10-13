#ifndef __R4_DATA_PROV_CPP__
#define __R4_DATA_PROV_CPP__

#include <memory>
#include <chrono>
#include <stdexcept>

#include "data_provider.h"
#include "reporter_controller.h"

IksReportDataProvider::IksReportDataProvider(const ServerInfoHolder& _srv_info): srv_info(_srv_info) {
	auto io = std::make_shared<boost::asio::io_service>();
	conn_mgr = std::make_shared<sz4::connection_mgr>(nullptr, srv_info.server, srv_info.port, "", io);
	base = std::make_shared<sz4::iks>(io, conn_mgr);

	io_thread = std::thread([this]() {
		try {

			conn_mgr->run();

		} catch(const std::exception& e) {
			std::lock_guard<std::mutex> guard(thread_mutex);
			std::cout << "IKS connection error: " << e.what() << std::endl;

			// we cannot do anything here
			// TODO: add handling
			throw;
		}
	}); // connection thread

	io_thread.detach();
}

IksReportDataProvider::~IksReportDataProvider() {
	Unsubscribe();
}


void IksReportDataProvider::SubscribeOnReport(const ReportInfo& report, ReportControllerBase* subscriber) {
	std::lock_guard<std::mutex> guard(thread_mutex);
	threads.clear();

	auto sub_tread = std::make_shared<SubscriberThread>(base, report, subscriber);
	threads.push_back(sub_tread);
}


void IksReportDataProvider::Unsubscribe() {
	std::lock_guard<std::mutex> guard(thread_mutex);
	threads.clear();
}

SubscriberThread::SubscriberThread(std::shared_ptr<sz4::iks> base, const ReportInfo report, ReportControllerBase* subscriber): param_vec() {
	for (const auto& param: report.params) {
		const sz4::param_info pi(report.config, param.name);
		param_vec.push_back(std::make_shared<ParamObserver>(base, subscriber, pi));
		param_vec.back()->register_self();
	}
}

SubscriberThread::~SubscriberThread() {
	for (auto p_o: param_vec) {
		p_o->deregister();
	}

	param_vec.clear();
}


using boost::system::error_code;


ParamObserver::ParamObserver(std::shared_ptr<sz4::iks> _base, ReportControllerBase* _subscriber, const sz4::param_info _pi): base(_base), subscriber(_subscriber), pi(_pi) {
	// cannot register here (shared_from_this requires at least one valid existing shared_ptr)
}

void ParamObserver::register_self() {
	auto sthis = shared_from_this();
	std::function<void(const error_code&)> error_callback = [sthis](const error_code& cb)
		{
			if (cb) sthis->onError(L"Error while subscribing on param");
		};

	// call initial update
	operator()();
	base->register_observer(sthis, {pi}, error_callback);
}

void ParamObserver::deregister() {
	auto sthis = shared_from_this();
	std::function<void(const error_code&)> error_callback = [sthis](const error_code& cb){ /* ignore */ };
	base->deregister_observer(sthis, {pi}, error_callback);
}

void ParamObserver::operator()() {
	using namespace std::chrono;
	const sz4::second_time_t last_acceptable_time = duration_cast<seconds>(system_clock::now().time_since_epoch()).count() - DATA_TIMEOUT;

	auto sthis = shared_from_this();
	std::function<void(const error_code& cb, const sz4::second_time_t& time_found)> time_callback = [sthis](const error_code& cb, const sz4::second_time_t& time_found) { sthis->getTime(cb, time_found); };

	base->search_data_left(pi, (unsigned int) -1, last_acceptable_time, SZARP_PROBE_TYPE::PT_SEC, time_callback);
}

void ParamObserver::onError(const std::wstring& error_msg) {
	subscriber->onSubscriptionError(error_msg);
}

void ParamObserver::getData(const error_code& cb, const std::vector<data_type>& values) {
	if (cb) {
		onError(L"Error while getting data from server");
		return;
	}

	if (values.size() >= 1) {
		if (subscriber) subscriber->onValueUpdate(std::get<1>(pi), values.back().avg());
	}
}

void ParamObserver::getTime(const error_code& cb, const sz4::second_time_t& time_found) {
	if (cb) {
		onError(L"Error while getting data time from server");
		return;
	}

	auto sthis = shared_from_this();
	std::function<void(const error_code& cb, const std::vector<data_type>&)> data_callback = [sthis](const error_code& cb, const std::vector<data_type>& dt) { sthis->getData(cb, dt); };

	base->get_weighted_sum(pi, sz4::time_just_before(time_found), time_found, SZARP_PROBE_TYPE::PT_SEC, data_callback);
}


#endif
