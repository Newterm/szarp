#include <unordered_set>
#include <boost/asio.hpp>
#include "sz4_iks_param_info.h"
#include "sz4_iks.h"
#include "sz4_iks_templ.h"

namespace sz4 {

iks::observer_reg::observer_reg(connection_mgr::loc_connection_ptr c, const std::string& name,
				param_info param, param_observer_f observer)
				: connection(c), name(name), param(param), observer(observer)
{
	error_sig_c = c->connection_error_sig.connect(std::bind(&observer_reg::on_error, this, std::placeholders::_1));
	cmd_sig_c = c->cmd_sig.connect(std::bind(&observer_reg::on_cmd, this, std::placeholders::_1
						, std::placeholders::_2, std::placeholders::_3));
}

void iks::observer_reg::on_cmd(const std::string& tag, IksCmdId, const std::string &data) {
	if (tag == "n" && data == name)
		(*observer)();
}

void iks::observer_reg::on_error(const boost::system::error_code& ec) {
	///notify connection error to observer
}

connection_mgr::loc_connection_ptr iks::connection_for_base(const std::wstring& prefix) {
	return m_connection_mgr->connection_for_base(prefix);
}

void iks::_register_observer(param_observer_f observer, std::vector<param_info> params, std::function<void(const boost::system::error_code&) > cb) {
	for (auto& p : params) {

		auto c = connection_for_base(p.prefix());
		if (!c)
			continue;

		std::basic_string<unsigned char> uname(SC::S2U(p.name()));
		std::string name(uname.begin(), uname.end());

		bool p_registered = std::any_of(m_observer_regs.begin(), m_observer_regs.end(),
				[&p] (observer_reg& e) { return e.param == p; });

		m_observer_regs.emplace_back(c, name, p, observer);

		if (p_registered)
			continue;

		std::stringstream ss;
		ss << "\"" << SC::S2U(p.name()) << "\"";

		c->send_command("param_subscribe", ss.str(), [] (const bs::error_code&,
								 const std::string&,
								 std::string&) {
			return IksCmdStatus::cmd_done;
		});
	}

	cb(make_error_code(bsec::success));
}

void iks::_deregister_observer(param_observer_f observer, std::vector<param_info> params, std::function<void(const boost::system::error_code&) > cb) {
	std::set<param_info> maybe_deregister, deregister;
	m_observer_regs.remove_if([&] (observer_reg& e) {
		if (e.observer == observer && std::find(params.begin(), params.end(), e.param) != params.end()) {
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

	for (auto& i : deregister) {
		auto c = connection_for_base(i.prefix());
		if (!c)
			continue;

		std::stringstream ss;
		ss << "\"" << SC::S2U(i.name()) << "\"";

		c->send_command("param_unsubscribe", ss.str(), [] (const bs::error_code&,
								   const std::string&,
								   std::string&) {
			return IksCmdStatus::cmd_done;
		});
	}

	cb(make_error_code(bsec::success));
}

void iks::_add_param(param_info param, std::function<void(const boost::system::error_code&)> cb) {
	auto c = connection_for_base(param.prefix());
	if (!c) {
		cb(make_error_code(ie::not_connected_to_peer));
		return;
	}

	c->add_param(param, [cb] (const bs::error_code& ec, const std::string&, std::string&) {
			cb(ec);
			return IksCmdStatus::cmd_done;
	});

}

void iks::_remove_param(param_info param, std::function<void(const boost::system::error_code&)> cb) {
	auto c = connection_for_base(param.prefix());
	if (!c) {
		cb(make_error_code(ie::not_connected_to_peer));
		return;
	}

	c->remove_param(param, [cb] (const bs::error_code& ec, const std::string&, std::string&) {
			cb(ec);
			return IksCmdStatus::cmd_done;
	});
}

iks::iks(std::shared_ptr<boost::asio::io_service> io, std::shared_ptr<connection_mgr> connection_mgr) : m_io(io), m_connection_mgr(connection_mgr) {}

void iks::register_observer(param_observer_f observer, const std::vector<param_info>& params, std::function<void(const boost::system::error_code&) > cb) {
	m_io->post(std::bind(&iks::_register_observer, shared_from_this(), observer, params, cb));
}

void iks::deregister_observer(param_observer_f observer, const std::vector<param_info>& v, std::function<void(const boost::system::error_code&) > cb) {
	m_io->post(std::bind(&iks::_deregister_observer, shared_from_this(), observer, v, cb));
}

void iks::add_param(const param_info& param, std::function<void(const boost::system::error_code&)> cb) {
	m_io->post(std::bind(&iks::_add_param, shared_from_this(), param, cb));
}

void iks::remove_param(const param_info& param, std::function<void(const boost::system::error_code&)> cb) {
	m_io->post(std::bind(&iks::_remove_param, shared_from_this(), param, cb));
}

template void iks::search_data_right(const param_info&, const nanosecond_time_t&, const nanosecond_time_t&, SZARP_PROBE_TYPE, std::function<void(const boost::system::error_code&, const nanosecond_time_t&)>);

template void iks::search_data_right(const param_info&, const second_time_t&, const second_time_t&, SZARP_PROBE_TYPE, std::function<void(const boost::system::error_code&, const second_time_t&)>);

template void iks::search_data_left(const param_info&, const nanosecond_time_t&, const nanosecond_time_t&, SZARP_PROBE_TYPE, std::function<void(const boost::system::error_code&, const nanosecond_time_t&)>);

template void iks::search_data_left(const param_info&, const second_time_t&, const second_time_t&, SZARP_PROBE_TYPE, std::function<void(const boost::system::error_code&, const second_time_t&)>);

template void iks::get_weighted_sum(const param_info& param, const nanosecond_time_t&, const nanosecond_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<int, nanosecond_time_t> >&) > cb);

template void iks::get_weighted_sum(const param_info& param, const nanosecond_time_t&, const nanosecond_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<short, nanosecond_time_t> >&) > cb);

template void iks::get_weighted_sum(const param_info& param, const nanosecond_time_t&, const nanosecond_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<double, nanosecond_time_t> >&) > cb);

template void iks::get_weighted_sum(const param_info& param, const second_time_t&, const second_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<int, second_time_t> >&) > cb);

template void iks::get_weighted_sum(const param_info& param, const second_time_t&, const second_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<short, second_time_t> >&) > cb);

template void iks::get_weighted_sum(const param_info& param, const second_time_t&, const second_time_t& , SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<double, second_time_t> >&) > cb);

}
/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
