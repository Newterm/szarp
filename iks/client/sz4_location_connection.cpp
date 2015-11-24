#include <boost/asio.hpp>

#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "szarp_config.h"
#include "iks_connection.h"
#include "sz4_location_connection.h"

namespace bp = boost::property_tree;
namespace bsec = boost::system::errc;

namespace sz4 {

void location_connection::connect_to_location() 
{
	auto self = shared_from_this();

	m_connection->send_command("connect" , m_location,
			[self] ( IksError error, const std::string& status, std::string& data ) {
		if ( error )
			return IksCmdStatus::cmd_done;

		if ( status != "k" ) {
			self->connection_error_sig(bsec::make_error_code(bsec::not_connected));
			return IksCmdStatus::cmd_done;
		}

		///XXX: 1. send defined params
		///XXX: 2. send cached

		self->connected_sig();
		self->m_connected = true;

		return IksCmdStatus::cmd_done;

	});
}

location_connection::location_connection(IPKContainer* container, boost::asio::io_service& io,
					const std::string& location, const std::string& server,
					const std::string& port)
					: m_container(container), m_location(location), 
					m_connection(std::make_shared<IksConnection>(io, server, port)),
					m_connected(false)
{
}

void location_connection::send_command(const std::string& cmd, const std::string& data, IksCmdCallback callback )
{
	///XXX: cache if not connected
	m_connection->send_command(cmd, data, callback);
}

void location_connection::send_command(IksCmdId id, const std::string& cmd, const std::string& data)
{
	///XXX: cache if not connected
	m_connection->send_command(id, cmd, data);
}

void location_connection::on_cmd(const std::string& tag, IksCmdId id, const std::string& data)
{
	cmd_sig(tag, id, data);	
}

void location_connection::on_connected()
{
	connect_to_location();
}

void location_connection::on_connection_error(const boost::system::error_code& ec) 
{
	connection_error_sig(ec);	
}

void location_connection::start_connecting() {
	namespace p = std::placeholders;

	auto self = shared_from_this();

	m_connection->connected_sig.connect(std::bind(&location_connection::on_connected, self));
	m_connection->connection_error_sig.connect(std::bind(&location_connection::on_connected, self));
	m_connection->cmd_sig.connect( std::bind(&location_connection::on_cmd, self, p::_1, p::_2, p::_3));

	m_connection->connect();
}

void location_connection::disconnect() {
	m_connection->disconnect();
}

}

/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
