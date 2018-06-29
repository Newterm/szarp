#include <boost/asio.hpp>

#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "conversion.h"
#include "szarp_config.h"

#include "liblog.h"

#include "szarp_base_common/lua_strings_extract.h"

#include "iks_connection.h"
#include "sz4_iks_param_info.h"
#include "sz4_location_connection.h"

#include "locations/error_codes.h"

#include "utils/ptree.h"

namespace bp = boost::property_tree;
namespace bs = boost::system;
namespace bsec = boost::system::errc;

namespace sz4 {

namespace {

struct visitor : public boost::static_visitor<> {
	location_connection& loc;
public:
	visitor(location_connection& loc) : loc(loc) {}
	template<class T> void operator()(T& t) const {
		loc.send_command(std::get<0>(t), std::get<1>(t), std::get<2>(t));
	}
};

}

void location_connection::send_cached() {
	for (auto& c : m_cmd_cache)
		boost::apply_visitor(visitor(*this), c);

	m_cmd_cache.clear();
}

void location_connection::connect_to_location() 
{
	auto self = shared_from_this();

	m_connection->send_command("connect" , m_location,
			[self] ( const bs::error_code& ec, const std::string& status, std::string& data ) {

		sz_log( 5 , "location_connection(%p)::connect_to_location error: %s"
		      , self.get() , ec.message().c_str() );

		if ( ec ) {
			self->connection_error_sig( ec );
			return IksCmdStatus::cmd_done;
		}

		if ( status != "k" ) {
			sz_log( 5 , "location_connection(%p)::connect_to_location not ok from server"
			        " error: %s , data: %s" , self.get(), status.c_str() , data.c_str() );

			self->connection_error_sig( make_iks_error_code( data ) );
			return IksCmdStatus::cmd_done;
		}

		sz_log( 10 , "location_connection(%p)::connected " , self.get() );

		self->m_connected = true;

		self->send_cached();

		self->connected_sig();

		return IksCmdStatus::cmd_done;

	});
}

location_connection::location_connection(IPKContainer* container, boost::asio::io_service& io,
					const std::string& location, const std::string& server,
					const std::string& port, const std::string& defined_param_prefix)
					: m_container(container), m_location(location), 
					m_connection(std::make_shared<IksConnection>(io, server, port)),
					m_defined_param_prefix(defined_param_prefix), m_connected(false)
{
	sz_log(10, "location_connection::location_connection(%p), m_connection(%p), location: %s"
	      , this, m_connection.get(), location.c_str() );
	      
} 

void location_connection::send_command(const std::string& cmd, const std::string& data, IksCmdCallback callback )
{
	if (m_connected)
		m_connection->send_command(cmd, data, callback);
	else
		m_cmd_cache.push_back(std::make_tuple(cmd, data, callback));
}

void location_connection::send_command(IksCmdId id, const std::string& cmd, const std::string& data)
{
	if (m_connected)
		m_connection->send_command(id, cmd, data);
	else
		m_cmd_cache.push_back(std::make_tuple(id, cmd, data));
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
	m_connected = false;
	connection_error_sig(ec);
}

void location_connection::start_connecting() {
	namespace p = std::placeholders;

	auto self = shared_from_this();

	m_connection->connected_sig.connect(std::bind(&location_connection::on_connected, self));
	m_connection->connection_error_sig.connect(std::bind(&location_connection::on_connection_error, self, p::_1));
	m_connection->cmd_sig.connect(std::bind(&location_connection::on_cmd, self, p::_1, p::_2, p::_3));

	m_connection->connect();
}

void location_connection::disconnect() {
	m_connection->disconnect();
}

}

/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
