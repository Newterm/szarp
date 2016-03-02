#include <boost/asio.hpp>

#include "iks_connection.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace bae = ba::error;
namespace bsec = boost::system::errc;

void IksConnection::schedule_keepalive() {
	auto self = shared_from_this();
	keepalive_timer.expires_from_now( boost::posix_time::seconds( 5 ) );
	keepalive_timer.async_wait([self] ( const bs::error_code& ec ) {
		if ( ec == bae::operation_aborted )
			return;

		self->send_command("are_you_there", "" , [self] ( const bs::error_code &ec
								      , const std::string& status
								      , std::string& data ) {

			if ( !ec && self->state == CONNECTED )
				self->schedule_keepalive();

			bs::error_code _ec;
			self->keepalive_timeout_timer.cancel(_ec);

			return IksCmdStatus::cmd_done;
		});

		self->keepalive_timeout_timer.expires_from_now( boost::posix_time::seconds( 10 ) );
		self->keepalive_timeout_timer.async_wait([self] ( const bs::error_code& ec ) {
				if ( ec == bae::operation_aborted )
						return;

				switch (self->state) {
					case CONNECTED:
						break;
					default:
						return;
				}

				auto _ec = make_error_code( bs::errc::stream_timeout );

				std::string empty;
				for ( auto& i : self->commands )
						i.second( _ec , "" , empty );
				self->commands.clear();

				self->connection_error_sig( _ec );
				self->schedule_reconnect();
		});

	});

}

void IksConnection::schedule_reconnect() {
	auto self = shared_from_this();

	self->disconnect();

	reconnect_timer.expires_from_now(boost::posix_time::seconds(1));
	reconnect_timer.async_wait([self] (const bs::error_code& ec) {
		if (ec == bae::operation_aborted)
			return;

		self->connect();
	});

	state = WAITING;
}

IksConnection::IksConnection( ba::io_service& io
							, const std::string& server
							, const std::string& port)
							: next_cmd_id(0)
							, socket( std::make_shared<TcpClientSocket>( io , server , port , *this ) )
							, keepalive_timer( io )
							, keepalive_timeout_timer( io )
							, reconnect_timer( io )
							, connect_timeout_timer( io )
{
}

IksCmdId IksConnection::send_command( const std::string& cmd
												, const std::string& data
												, IksCmdCallback callback )
{
	IksCmdId id = next_cmd_id++;

	send_command( id , cmd , data );
	commands[id] = callback;

	return id;
}

void IksConnection::send_command( IksCmdId id
								, const std::string& cmd
								, const std::string& data )
{
	std::ostringstream os;

	os << cmd << " " << id;
	if ( data.size() )
		os << " " << data;
	os << "\n";

	socket->write( os.str() );
}

void IksConnection::remove_command(IksCmdId id)
{
	commands.erase(id);
}

void IksConnection::handle_read_line( ba::streambuf& buf )
{
	std::istream is( &buf );
	std::string tag;
	std::string data;
	IksCmdId id;

	std::getline(is, tag, ' ');
	is >> id;
	is.ignore(1);
	std::getline(is, data);

	if ( !is ) {
		auto ec = make_error_code( iks_client_error::invalid_server_response );

		std::string empty;
		for ( auto& i : commands )
			i.second( make_error_code( iks_client_error::invalid_server_response )
					, ""
					, empty );
		commands.clear();

		connection_error_sig( ec );
		schedule_reconnect();
		return;
	}

	auto i = commands.find(id);
	if ( i != commands.end() ) {
		if ( i->second( make_error_code( bs::errc::success ), tag , data ) == cmd_done)
			commands.erase(i);
	} else
		cmd_sig( tag, id, data );
}

void IksConnection::handle_error( const bs::error_code& ec )
{
	if ( ec == bae::operation_aborted )
		return;
	
	switch (state) {
		case CONNECTED:
			break;
		case CONNECTING:
			schedule_reconnect();
			return;
		case WAITING:
			assert( false );
			break;
	}

	std::string empty;
	for ( auto& i : commands ) 
		i.second( ec , "" , empty );
	commands.clear();

	connection_error_sig(ec);

	bs::error_code _ec;
	keepalive_timer.cancel(_ec);

	schedule_reconnect();
}

void IksConnection::handle_connected()
{
	bs::error_code _ec;
	connect_timeout_timer.cancel(_ec);

	state = CONNECTED;
	connected_sig();

	schedule_keepalive();
}

void IksConnection::handle_disconnected()
{
	///XXX:
}

void IksConnection::connect() 
{
	auto self = shared_from_this();
	state = CONNECTING;

	socket->connect();

	connect_timeout_timer.expires_from_now( boost::posix_time::seconds( 20 ) );
	connect_timeout_timer.async_wait([self] ( const bs::error_code& ec ) {
			if ( ec == bae::operation_aborted )
					return;

			switch (self->state) {
				case CONNECTING:
					self->schedule_reconnect();
					break;
				default:
					break;
			}
	});
}

void IksConnection::disconnect() {
	socket->close();		
}

IksConnection::~IksConnection()
{
	socket->close();
}

/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
