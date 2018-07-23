#ifndef __LIVE_IKS_CACHE__
#define __LIVE_IKS_CACHE__

#include "parhub_poller.h"

#include <unordered_map>
#include <mutex>
#include <list>
#include <boost/optional.hpp>
#include "sz4/base.h"

class Observer {
public:
	virtual ~Observer() {}
};

class LiveObserver: public Observer {
public:
	LiveObserver(size_t _ind, std::function<void()> _cb): ipc_ind(_ind), cb(_cb) {}
	const size_t ipc_ind;
	const std::function<void()> cb;
};

class LiveCache: public ParhubSubscriber {
	using Observers = std::list<std::weak_ptr<LiveObserver>>;
	std::unordered_map<size_t, std::pair<double, Observers>> live_callbacks;
	std::unordered_map<std::string, size_t> params_inds;
	std::unordered_map<size_t, std::string> params_names;
	std::unordered_map<size_t, size_t> params_prec_adjs;

	mutable std::recursive_mutex cb_mutex;

public:
	void param_value_changed(size_t ipc_ind, const szarp::ParamValue& value) override;

	boost::optional<std::string> get_param_name(size_t ind) const;
	boost::optional<size_t> get_param_ipc_ind(const std::string& pname) const;

	bool has_param(const std::string& pname) const;
	double get_value(size_t ind) const;
	double get_value(const std::string& pname) const;

	std::shared_ptr<LiveObserver> add_observer(const std::string& pname, size_t ipc_ind, size_t prec, std::function<void()> cb, std::function<double()> init_value);
};

#endif
