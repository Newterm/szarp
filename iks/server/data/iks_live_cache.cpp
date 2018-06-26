#include "iks_live_cache.h"
#include "parhub_poller.h"
#include "sz4/util.h"

void LiveCache::param_value_changed(size_t ipc_ind, const szarp::ParamValue& value) {
	// unordered_map lookup is fast enough for us to double-check not to block mutex if there is no subscriber (we MIGHT rarely miss first update on param about to be subscribed).
	if (!live_callbacks.count(ipc_ind)) return;

	std::lock_guard<std::recursive_mutex> cb_guard(cb_mutex);

	auto param_it = live_callbacks.find(ipc_ind);
	if (param_it == live_callbacks.end()) return;

	auto prec_adj_it = params_prec_adjs.find(ipc_ind);
	if (prec_adj_it == params_prec_adjs.end()) return;

	auto vp = sz4::cast_param_value<double>(value, prec_adj_it->second);
	param_it->second.first = vp.value;

	auto end = param_it->second.second.end();
	for (auto ob_it = param_it->second.second.begin(); ob_it != end;) {
		if (ob_it->expired()) {
			ob_it = param_it->second.second.erase(ob_it);
		} else {
			ob_it->lock()->cb();
			++ob_it;
		}
	}

	if (param_it->second.second.size() == 0) {
		auto name_it = params_names.find(ipc_ind);
		if (name_it == params_names.end()) return;

		auto ind_it = params_inds.find(name_it->second);
		if (ind_it != params_inds.end()) params_inds.erase(ind_it);

		auto prec_adj_it = params_prec_adjs.find(ipc_ind);
		if (prec_adj_it != params_prec_adjs.end()) params_prec_adjs.erase(prec_adj_it);

		params_names.erase(name_it);
		live_callbacks.erase(param_it);
	}
}

boost::optional<std::string> LiveCache::get_param_name(size_t ind) const {
	auto param_it = params_names.find(ind);
	if (param_it == params_names.end()) return boost::none;
	return param_it->second;
}

boost::optional<size_t> LiveCache::get_param_ipc_ind(const std::string& pname) const {
	auto name_it = params_inds.find(pname);
	if (name_it == params_inds.end()) return boost::none;
	return name_it->second;
}

bool LiveCache::has_param(const std::string& pname) const {
	return params_inds.find(pname) != params_inds.end();
}

double LiveCache::get_value(size_t ind) const {
	std::lock_guard<std::recursive_mutex> cb_guard(cb_mutex);
	auto param_it = live_callbacks.find(ind);
	if (param_it == live_callbacks.end()) return sz4::no_data<double>();

	return param_it->second.first;
}

double LiveCache::get_value(const std::string& pname) const {
	if (auto ipc_ind = get_param_ipc_ind(pname)) {
		return get_value(*ipc_ind);
	}
	
	return sz4::no_data<double>();
}

std::shared_ptr<LiveObserver> LiveCache::add_observer(const std::string& pname, size_t ipc_ind, size_t prec, std::function<void()> cb, std::function<double()> init_value) {
	std::lock_guard<std::recursive_mutex> cb_guard(cb_mutex);
	auto observer_ptr = std::shared_ptr<LiveObserver>(new LiveObserver(ipc_ind, cb));
	auto c_it = live_callbacks.find(observer_ptr->ipc_ind);
	if (c_it == live_callbacks.end()) {
		live_callbacks.insert({ipc_ind, {init_value(), {observer_ptr}}});
		params_inds.insert({pname, ipc_ind});
		params_names.insert({ipc_ind, pname});
		params_prec_adjs.insert({ipc_ind, pow(10.0, prec)});
	} else {
		c_it->second.second.push_back(observer_ptr);
	}

	return observer_ptr;
}
