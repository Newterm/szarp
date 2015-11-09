#include "iks_connection.h"

IksConnection::IksConnection( boost::asio::io_service& io
							, const std::string& server
							, const std::string& port)
							: next_cmd_id(0)
							, socket( std::make_shared<TcpClientSocket>( io , server , port , *this ) )
{

}

IksConnection::CmdId IksConnection::send_command( const std::string& cmd
												, const std::string& data
												, IksConnection::CmdCallback callback )
{
	CmdId id = next_cmd_id++;

	send_command( id , cmd , data );
	commands[id] = callback;

	return id;
}

void IksConnection::send_command( CmdId id
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

void IksConnection::remove_command(CmdId id)
{
	commands.erase(id);
}

void IksConnection::handle_read_line( boost::asio::streambuf& buf )
{
	std::istream is( &buf );
	std::string tag;
	std::string data;
	CmdId id;

	std::getline(is, tag, ' ');
	is >> id;
	is.ignore(1);
	std::getline(is, data);

	if ( !is ) {
		std::string empty;
		for ( auto i : commands )
			i.second( Error::connection_error , "" , empty );
		socket->connect();
		return;
	}

	auto i = commands.find(id);
	if ( i != commands.end() )
		i->second( Error::no_error , tag , data );
	else
		cmd_sig( tag, id, data );
}

void IksConnection::handle_error( const boost::system::error_code& ec )
{
	std::string empty;
	for ( auto& i : commands ) 
		i.second( Error::connection_error , "" , empty );

	commands.clear();

	connection_error_sig(ec);
}

void IksConnection::handle_connected()
{
	connected_sig();
}

void IksConnection::handle_disconnected()
{
	///XXX:
}

void IksConnection::disconnect() {
	socket->close();		
}

IksConnection::~IksConnection()
{
	socket->close();
}

/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
