#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "conversion.h"
#include "szarp_config.h"
#include "iks_connection.h"
#include "sz4_connection_mgr.h"
#include "sz4_location_connection.h"

#include "locations/error_codes.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace bae = ba::error;
namespace bsec = boost::system::errc;

namespace sz4 {

class connection_mgr::timer : public ba::deadline_timer {
public:
	timer(ba::io_service& io) : ba::deadline_timer(io) 
	{}
};

bool connection_mgr::parse_remotes(const std::string &data, std::vector<remote_entry>& remotes) {

	namespace bp = boost::property_tree;

	try {
		std::stringstream ss(data);
		bp::ptree json;
		bp::json_parser::read_json(ss , json);
		for (auto itr = json.begin(); itr!=json.end(); ++itr)
			if (itr->second.get<std::string>("type") == "szbase") {
				auto& loc = itr->first;
				std::string prefix(
					std::find(
						loc.rbegin(),
						loc.rend(),
						':').base(),
					loc.end());

				remotes.emplace_back(
					SC::U2S((unsigned char*)prefix.c_str()),
					loc);
			}
	} catch (const bp::ptree_error& e) {
		return false;
	}
	
	return true;
}

void connection_mgr::connect() {
	m_connection = std::make_shared<IksConnection>(*m_io, m_address, m_port);

	auto self = shared_from_this();

	namespace p = std::placeholders;

	m_connection->connected_sig.connect(std::bind(&connection_mgr::on_connected, self));
	m_connection->connection_error_sig.connect(std::bind(&connection_mgr::on_error, self, p::_1));
	m_connection->cmd_sig.connect(std::bind(&connection_mgr::on_cmd, self, p::_1, p::_2, p::_3));

	m_connection->connect();
}

void connection_mgr::connect_location(const std::wstring& name, const std::string& location) {

	auto citr = m_location_connections.find(name);
	if (citr != m_location_connections.end()) {
		citr->second->disconnect();
		m_location_connections.erase(citr);
	}

	auto ritr = std::find_if(m_remotes.begin(), m_remotes.end(),
		[&] (const remote_entry& e) { return e.first == name; });
	if (ritr != m_remotes.end())
		ritr->second = location;
	else
		m_remotes.emplace_back(name, location);	

	auto c = std::make_shared<location_connection>(m_ipk_container, *m_io, location, m_address, m_port);
	auto self = shared_from_this();

	c->connection_error_sig.connect(std::bind(&connection_mgr::on_location_error, self, name, std::placeholders::_1));
	c->connected_sig.connect(std::bind(&connection_mgr::on_location_connected, self, name));

	m_location_connections.insert(std::make_pair(name, c));

	c->start_connecting();
}

void connection_mgr::disconnect_location(const std::wstring& name) {
	auto locr = std::find_if(m_remotes.begin(), m_remotes.end(),
		[&] (const remote_entry& e) { return e.first == name; });
	if (locr == m_remotes.end())
		return;

	auto loci = m_location_connections.find(name);
	if (loci != m_location_connections.end()) {
		loci->second->disconnect();	
		m_location_connections.erase(loci);
	}

	m_remotes.erase(locr);

	disconnected_location_sig(name);
}


void connection_mgr::schedule_reconnect() {
	if (m_connection) {
		m_connection->disconnect();
		m_connection.reset();
	}

	auto self = shared_from_this();
	m_reconnect_timer->expires_from_now(boost::posix_time::seconds(2));
	m_reconnect_timer->async_wait([self] (const bs::error_code& ec) {
		if (ec == bae::operation_aborted)
			return;

		self->connect();
	});
}

void connection_mgr::on_connected() {
	connected_sig();

	auto self = shared_from_this();

	m_connection->send_command("list_remotes", "" , [self] ( const bs::error_code &ec
								, const std::string& status
								, std::string& data ) {

		bs::error_code _ec = ec;
	
		if (!_ec && status != "k")
			_ec = make_iks_error_code(data);

		if (!_ec) {
			std::vector<remote_entry> remotes;
			if (self->parse_remotes(data, remotes))
				for (auto& e : remotes)
					self->connect_location(e.first, e.second);
			else
				_ec = make_error_code(iks_client_error::invalid_server_response);
		}

		if (_ec) {
			self->on_error(_ec);
			self->schedule_reconnect();
		}

		return IksCmdStatus::cmd_done;
	});
}

void connection_mgr::on_error(const boost::system::error_code& ec) {
	if (ec != ba::error::operation_aborted) {
		connection_error_sig(ec);
		schedule_reconnect();
	}
}

void connection_mgr::on_cmd(const std::string& tag, IksCmdId, const std::string& data) {
	namespace bp = boost::property_tree;

	if (tag == "remote_added")
		try {
			bp::ptree json;
			std::stringstream ss(data);
			bp::json_parser::read_json(ss, json);

			if (json.get<std::string>("type") != "szbase")
				return;
			
			std::string location = json.get<std::string>("tag");
			std::wstring name = SC::U2S((unsigned char*)json.get<std::string>("name").c_str());

			connect_location(name, location);
		} catch (bp::ptree_error &e) {
			//XXX:
		}
	else if (tag == "remote_removed") {
		auto locr = std::find_if(m_remotes.begin(), m_remotes.end(),
			[&] (const remote_entry& e) { return e.second == data; });
			
		if (locr != m_remotes.end())
			disconnect_location(locr->first);
	}
}

void connection_mgr::on_location_connected(std::wstring name) {
	connected_location_sig(name);
}

void connection_mgr::on_location_error(std::wstring name, const bs::error_code& ec) {
	error_location_sig(name, ec);

	auto locr = std::find_if(m_remotes.begin(), m_remotes.end(),
		[&] (const remote_entry& e) { return e.first == name; });
	if (locr == m_remotes.end())
		return;

	connect_location(name, locr->second);
}

connection_mgr::connection_mgr(IPKContainer *conatiner,
				const std::string& address,
				const std::string& port,
				std::shared_ptr<ba::io_service> io)
				:
				m_ipk_container(conatiner),
				m_address(address),
				m_port(port),
				m_reconnect_timer(std::make_shared<timer>(*io)),
				m_io(io) {

}

connection_mgr::loc_connection_ptr connection_mgr::connection_for_base(const std::wstring& prefix) {
	auto i = m_location_connections.find(prefix);
	return i != m_location_connections.end() ? i->second : loc_connection_ptr();
}

void connection_mgr::run() {
	connect();
	m_io->run();
}

}
/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */

