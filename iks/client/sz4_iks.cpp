#include "sz4_iks.h"
#include "sz4_iks_templ.h"

namespace sz4 {

IksClient* iks::client_for_param(TParam *param) {
	auto i = cients.find(param->GetSzarpConfig()->GetPrefix());
	return i != cients.end() ? client.second : NULL;
}

void iks::register_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(error&) > cb) {
	auto count = std::make_shared<int>(0);
	for (auto p : params) {
		auto c = client_for_param(p);
		if (!c)
			continue;

		std::basic_string<unsigned char> uname(p->GetName());
		std::string name(uname.begin(), uname.end());

		bool p_registered = std::any_of(m_registrations.begin(), m_registrations.end(),
				[&i] (registration& e) { return e.param == i->param; });
		observers.emplace_back({ c , name , p , observer });

		if (p_registered)
			continue;

		*count += 1;

		std::stringstream ss;
		ss << "\"" << SC::S2U(reg.second->GetName()) << "\"";

		c->send_command("subscribe_param", ss.str(), [count] (IksClient::error, const std::string& , std::string&) {
			if (--*count == 0)
				cb(error::no_error);

			return IksClient::cmd_done;
		});
	}

	if (*count == 0)
		cb(error::no_error);

}

void iks::deregister_observer(param_observer *observer, const std::vector<TParam*>& , std::function<void(error&) > cb) {
	auto end = std::remove_if(m_registrations.begin(), m_registrations.end(),
			[observer] (registration& e) { return e.observer == observer });

	std::unordered_set<TParam*> params_to_deregister;
	for (auto i = end; i != m_registrations.end(); i++)
		if (std::none_of(m_registrations.begin(), m_registrations.end(),
				[&i] (registration& e) { return e.param == i->param; })) 
			params_to_deregister.insert(i->param);	

	auto count = std::make_shared<int>(0);

	for (auto& i : params_to_deregister) {
		auto c = client_for_param(*i);
		if (!c)
			continue;

		*count += 1;
		
		std::stringstream ss;
		ss << "\"" << SC::S2U(reg.second->GetName()) << "\"";

		c->send_command("unsubscribe_param", ss.str(), [count] (IksClient::error, const std::string& , std::string&) {
			if (--*count == 0)
				cb(error::no_error);

			return IksClient::cmd_done;
		});
	}

	if (*count == 0)
		cb(error::no_error);

}

void iks::add_param(TParam* param, std::function<void(error&) > result) {
//	m_impl->add_param(param, result);
}

void iks::remove_param(TParam* param, std::function<void(error&) > result) {
//	m_impl->remove_param(param, result);
}


}
