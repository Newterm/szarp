#include <unordered_set>

#include "sz4_iks.h"
#include "sz4_iks_templ.h"

namespace sz4 {

IksClient* iks::client_for_param(TParam *param) {
	auto i = m_clients.find(param->GetSzarpConfig()->GetPrefix());
	return i != m_clients.end() ? i->second : nullptr;
}

void iks::register_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(const error&) > cb) {
	auto count = std::make_shared<int>(0);
	for (auto p : params) {
		auto c = client_for_param(p);
		if (!c)
			continue;

		std::basic_string<unsigned char> uname(SC::S2U(p->GetName()));
		std::string name(uname.begin(), uname.end());

		bool p_registered = std::any_of(m_observer_regs.begin(), m_observer_regs.end(),
				[&p] (observer_reg& e) { return e.param == p; });
		m_observer_regs.push_back(observer_reg({ c , name , p , observer }));

		if (p_registered)
			continue;

		*count += 1;

		std::stringstream ss;
		ss << "\"" << SC::S2U(p->GetName()) << "\"";

		c->send_command("subscribe_param", ss.str(), [count, cb] (IksClient::Error, const std::string& , std::string&) {
			if (--*count == 0)
				cb(error::no_error);

			return IksClient::cmd_done;
		});
	}

	if (*count == 0)
		cb(error::no_error);

}

void iks::deregister_observer(param_observer *observer, const std::vector<TParam*>& , std::function<void(const error&) > cb) {
	auto end = std::remove_if(m_observer_regs.begin(), m_observer_regs.end(),
			[observer] (observer_reg& e) { return e.observer == observer; });

	std::unordered_set<TParam*> params_to_deregister;
	for (auto i = end; i != m_observer_regs.end(); i++)
		if (std::none_of(m_observer_regs.begin(), m_observer_regs.end(),
				[&i] (observer_reg& e) { return e.param == i->param; })) 
			params_to_deregister.insert(i->param);	

	auto count = std::make_shared<int>(0);

	for (auto& i : params_to_deregister) {
		auto c = client_for_param(&*i);
		if (!c)
			continue;

		*count += 1;
		
		std::stringstream ss;
		ss << "\"" << SC::S2U(i->GetName()) << "\"";

		c->send_command("unsubscribe_param", ss.str(), [count, cb] (IksClient::Error, const std::string& , std::string&) {
			if (--*count == 0)
				cb(error::no_error);

			return IksClient::cmd_done;
		});
	}

	if (*count == 0)
		cb(error::no_error);

}

/*
void iks::add_param(TParam* param, std::function<void(const error&) > result) {
//	m_impl->add_param(param, result);
}

void iks::remove_param(TParam* param, std::function<void(const error&) > result) {
//	m_impl->remove_param(param, result);
}
*/


}
