#ifndef __R4_DATA_PROV_H__
#define __R4_DATA_PROV_H__

#include <thread>
#include <atomic>
#include <mutex>

#include <memory>

#include "../../libSzarp2/include/sz4/defs.h"
#include "../../libSzarp2/include/sz4/time.h"
#include "../../iks/client/sz4_iks.h"
#include "../../iks/client/sz4_connection_mgr.h"

#include "report.h"

class ReportControllerBase;
class SubscriberThread;
class atomic_bool;

class IksReportDataProvider {
	ServerInfoHolder srv_info;

	std::thread io_thread;
	std::shared_ptr<sz4::connection_mgr> conn_mgr;
	std::shared_ptr<sz4::iks> base;

	std::vector<std::shared_ptr<SubscriberThread>> threads;
	std::mutex thread_mutex;

public:
	IksReportDataProvider(const ServerInfoHolder& _srv_info);
	~IksReportDataProvider();

	IksReportDataProvider(const IksReportDataProvider& other) = delete;
	IksReportDataProvider(IksReportDataProvider&& other) = delete;

	void Unsubscribe();
	void SubscribeOnReport(const ReportInfo& report, ReportControllerBase* subscriber);

};


class atomic_bool {
	std::atomic<bool> flag;
public:
	atomic_bool(bool _flag = false) { flag.store(_flag); }
	atomic_bool(const atomic_bool& other) {
		flag.store(other.flag.load());
	}

	~atomic_bool() = default;

	atomic_bool(atomic_bool&& other) {
		flag.store(other.flag.load());
	}

	atomic_bool& operator=(const atomic_bool& other) {
		flag.store(other.flag.load());
		return *this;
	}

	atomic_bool& operator=(atomic_bool&& other) {
		flag.store(other.flag.load());
		return *this;
	}

	atomic_bool& operator=(bool _flag) {
		flag.store(_flag);
		return *this;
	}

	operator bool() const { return flag.load(); }
};

class ParamObserver: public sz4::param_observer_, public std::enable_shared_from_this<ParamObserver> {
	std::shared_ptr<sz4::iks> base;
	ReportControllerBase* subscriber;
	sz4::param_info pi;

	const unsigned int DATA_TIMEOUT = 600;

	using data_type = sz4::weighted_sum<double, sz4::second_time_t>;

public:
	ParamObserver(std::shared_ptr<sz4::iks>, ReportControllerBase*, const sz4::param_info);
	~ParamObserver() = default;

	void getData(const boost::system::error_code&, const std::vector<data_type>&, const sz4::param_info);
	void getTime(const boost::system::error_code&, const sz4::second_time_t&, const sz4::param_info);

	void onError(const std::wstring&);

	void register_self();
	void deregister();

	void operator()() override;
};

class SubscriberThread {
	std::vector<std::shared_ptr<ParamObserver>> param_vec;
	
public:
	SubscriberThread(std::shared_ptr<sz4::iks> base, const ReportInfo report, ReportControllerBase* subscriber);
	~SubscriberThread();

};
 

#endif
