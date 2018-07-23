#include <unordered_set>
#include <boost/asio.hpp>
#include "sz4_iks_param_info.h"
#include "sz4_iks.h"
#include "sz4_iks_templ.h"

namespace sz4 {

iks::observer_reg::observer_reg(std::shared_ptr<connection_mgr> conn_mgr, const std::string& name,
				param_info param)
				: conn_mgr(conn_mgr), name(name), param(param)
{
	connected_sig_c = conn_mgr->connected_location_sig.connect(std::bind(&observer_reg::on_connected, this, std::placeholders::_1));

	connect();
}

iks::observer_reg::~observer_reg() {
	auto c = conn_mgr->connection_for_base(param.prefix());
	if (!c)
		return;


	std::stringstream ss;
	ss << "\"" << SC::S2U(param.name()) << "\"";

	c->send_command("param_unsubscribe", ss.str(), [] (const bs::error_code&,
							   const std::string&,
							   std::string&) {
		return IksCmdStatus::cmd_done;
	});
}

void iks::observer_reg::on_cmd(const std::string& tag, IksCmdId, const std::string &data) {
	if (tag == "n" && data == name)
		for (auto& observer : observers)
			(*observer)();
}

void iks::observer_reg::on_connected(std::wstring prefix) {
	if (param.prefix() != prefix)
		return;

	connect();
}

void iks::observer_reg::on_error(const boost::system::error_code& ec) {
	///notify connection error to observer
}

void iks::observer_reg::connect() {
	auto c = conn_mgr->connection_for_base(param.prefix());
	if (!c)
		return;

	error_sig_c = c->connection_error_sig.connect(std::bind(&observer_reg::on_error, this, std::placeholders::_1));
	cmd_sig_c = c->cmd_sig.connect(std::bind(&observer_reg::on_cmd, this, std::placeholders::_1
						, std::placeholders::_2, std::placeholders::_3));

	std::stringstream ss;
	ss << "\"" << SC::S2U(param.name()) << "\"";

	c->send_command("param_subscribe", ss.str(), [] (const bs::error_code&,
							 const std::string&,
							 std::string&) {
		return IksCmdStatus::cmd_done;
	});
}

connection_mgr::loc_connection_ptr iks::connection_for_base(const std::wstring& prefix) {
	return m_connection_mgr->connection_for_base(prefix);
}

void iks::_register_observer(param_observer_f observer, std::vector<param_info> params, std::function<void(const boost::system::error_code&) > cb) {
	for (auto& p : params) {
		auto i = m_observer_regs.find(p);

		if (i == m_observer_regs.end()) {
			std::basic_string<unsigned char> uname(SC::S2U(p.name()));
			std::string name(uname.begin(), uname.end());

			i = m_observer_regs.insert(
					std::make_pair(
					            p,
					            std::make_shared<observer_reg>(m_connection_mgr, name, p))).first;
		}

		i->second->observers.push_back(observer);
	}

	cb(make_error_code(bsec::success));
}

void iks::_deregister_observer(param_observer_f observer, std::vector<param_info> params, std::function<void(const boost::system::error_code&) > cb) {
	for (auto& param : params) {
		auto i = m_observer_regs.find(param);
		if (i == m_observer_regs.end())
			continue;

		auto o_reg = i->second;

		auto j = std::find(o_reg->observers.begin(), o_reg->observers.end(), observer);	
		if (j == o_reg->observers.end())
			continue;

		o_reg->observers.erase(j);

		if (!o_reg->observers.size())
			m_observer_regs.erase(i);
	}

	cb(make_error_code(bsec::success));
}

iks::iks(std::shared_ptr<boost::asio::io_service> io, std::shared_ptr<connection_mgr> connection_mgr) : m_io(io), m_connection_mgr(connection_mgr) {}

void iks::register_observer(param_observer_f observer, const std::vector<param_info>& params, std::function<void(const boost::system::error_code&) > cb) {
	m_io->post(std::bind(&iks::_register_observer, shared_from_this(), observer, params, cb));
}

void iks::deregister_observer(param_observer_f observer, const std::vector<param_info>& v, std::function<void(const boost::system::error_code&) > cb) {
	m_io->post(std::bind(&iks::_deregister_observer, shared_from_this(), observer, v, cb));
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
