#include <unordered_set>
#include <boost/asio.hpp>
#include "sz4_iks.h"
#include "sz4_iks_templ.h"

namespace sz4 {

iks::observer_reg::observer_reg(connection_mgr::loc_connection_ptr c, const std::string& name,
				TParam* param, param_observer* observer)
				: connection(c), name(name), param(param), observer(observer)
{
	error_sig_c = c->connection_error_sig.connect(std::bind(&observer_reg::on_error, this, std::placeholders::_1));
	cmd_sig_c = c->cmd_sig.connect(std::bind(&observer_reg::on_cmd, this, std::placeholders::_1
						, std::placeholders::_2, std::placeholders::_3));
}

void iks::observer_reg::on_cmd(const std::string& tag, IksCmdId, const std::string &data) {
	if (tag == "n" && data == name)
		observer->param_data_changed(param);
}

void iks::observer_reg::on_error(const boost::system::error_code& ec) {
	///notify connection error to observer
}

connection_mgr::loc_connection_ptr iks::connection_for_param(TParam *p) {
	return m_connection_mgr->connection_for_param(p);
}

void iks::_register_observer(param_observer *observer, std::vector<TParam*> params, std::function<void(const boost::system::error_code&) > cb) {
	auto count = std::make_shared<int>(0);
	for (auto p : params) {

		auto c = connection_for_param(p);
		if (!c)
			continue;

		std::basic_string<unsigned char> uname(SC::S2U(p->GetName()));
		std::string name(uname.begin(), uname.end());

		bool p_registered = std::any_of(m_observer_regs.begin(), m_observer_regs.end(),
				[&p] (observer_reg& e) { return e.param == p; });

		m_observer_regs.emplace_back(c, name, p, observer);

		if (p_registered)
			continue;

		*count += 1;

		std::stringstream ss;
		ss << "\"" << SC::S2U(p->GetName()) << "\"";

		c->send_command("param_subscribe", ss.str(), [count, cb] (const bs::error_code&,
									  const std::string&,
									  std::string&) {
			if (--*count == 0)
				cb(make_error_code(bsec::success));

			return IksCmdStatus::cmd_done;
		});
	}

	if (*count == 0)
		cb(make_error_code(bsec::success));

}

void iks::_deregister_observer(param_observer *observer, std::vector<TParam*> params, std::function<void(const boost::system::error_code&) > cb) {
	std::unordered_set<TParam*> maybe_deregister, deregister;
	m_observer_regs.remove_if([observer, &maybe_deregister] (observer_reg& e) {
		if (e.observer == observer) {
			maybe_deregister.insert(e.param);
			return true;
		} else
			return false;
	});

	for (auto& p : maybe_deregister) {
		if (std::none_of(m_observer_regs.begin(), m_observer_regs.end(),
				[&p] (observer_reg& e) { return e.param == p; })) 
			deregister.insert(p);
	}

	auto count = std::make_shared<int>(0);

	for (auto& i : deregister) {
		auto c = connection_for_param(&*i);
		if (!c)
			continue;

		*count += 1;
		
		std::stringstream ss;
		ss << "\"" << SC::S2U(i->GetName()) << "\"";

		c->send_command("param_unsubscribe", ss.str(), [count, cb] (const bs::error_code&,
									    const std::string&,
									    std::string&) {
			if (--*count == 0)
				cb(make_error_code(bsec::success));

			return IksCmdStatus::cmd_done;
		});
	}

	if (*count == 0)
		cb(make_error_code(bsec::success));
}


iks::iks(std::shared_ptr<boost::asio::io_service> io, std::shared_ptr<connection_mgr> connection_mgr) : m_io(io), m_connection_mgr(connection_mgr) {}

void iks::register_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(const boost::system::error_code&) > cb) {
	m_io->post(std::bind(&iks::_register_observer, shared_from_this(), observer, params, cb));
}

void iks::deregister_observer(param_observer *observer, const std::vector<TParam*>& v, std::function<void(const boost::system::error_code&) > cb) {
	m_io->post(std::bind(&iks::_deregister_observer, shared_from_this(), observer, v, cb));
}

template void iks::search_data_right(TParam*, const nanosecond_time_t&, const nanosecond_time_t&, SZARP_PROBE_TYPE, std::function<void(const boost::system::error_code&, const nanosecond_time_t&)>);

template void iks::search_data_right(TParam*, const second_time_t&, const second_time_t&, SZARP_PROBE_TYPE, std::function<void(const boost::system::error_code&, const second_time_t&)>);

template void iks::search_data_left(TParam*, const nanosecond_time_t&, const nanosecond_time_t&, SZARP_PROBE_TYPE, std::function<void(const boost::system::error_code&, const nanosecond_time_t&)>);

template void iks::search_data_left(TParam*, const second_time_t&, const second_time_t&, SZARP_PROBE_TYPE, std::function<void(const boost::system::error_code&, const second_time_t&)>);


template void iks::get_weighted_sum(TParam* param, const nanosecond_time_t&, const nanosecond_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<int, nanosecond_time_t> >&) > cb);

template void iks::get_weighted_sum(TParam* param, const nanosecond_time_t&, const nanosecond_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<short, nanosecond_time_t> >&) > cb);

template void iks::get_weighted_sum(TParam* param, const nanosecond_time_t&, const nanosecond_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<double, nanosecond_time_t> >&) > cb);

template void iks::get_weighted_sum(TParam* param, const second_time_t&, const second_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<int, second_time_t> >&) > cb);

template void iks::get_weighted_sum(TParam* param, const second_time_t&, const second_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<short, second_time_t> >&) > cb);

template void iks::get_weighted_sum(TParam* param, const second_time_t&, const second_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<double, second_time_t> >&) > cb);

}
/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
