#include "iks_connection.h"

IksConnection::IksConnection( boost::asio::io_service& io
		    , const std::string& server
		    , const std::string& port
		    , IksCmdReceiver* receiver )
		    : next_cmd_id(0)
		    , socket( std::make_shared<TcpClientSocket>( io , server , port , *this ) )
		    , receiver( receiver )
{

}

IksConnection::CmdId IksConnection::send_command(
		const std::string& cmd,
		const std::string& data,
		CmdCallback callback )
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
		receiver->on_cmd(tag, id, data);
}

void IksConnection::handle_error( const boost::system::error_code& ec )
{
	std::string empty;
	for ( auto& i : commands ) 
		i.second( Error::connection_error , "" , empty );

	commands.clear();
}

void IksConnection::handle_connected()
{
	receiver->on_connected();	
}

void IksConnection::handle_disconnected()
{
//	receiver->on_connected();	
}

IksConnection::~IksConnection()
{
	socket->close();
}
