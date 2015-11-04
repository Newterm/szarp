#include "iks_client.h"

IksClient::IksClient( boost::asio::io_service& io
		    , const std::string& server
		    , const std::string& port )
		    : next_cmd_id(0)
		    , socket( std::make_shared<TcpClientSocket>( io , server , port , *this ) )
{

}

IksClient::CmdId IksClient::send_command(
		const std::string& cmd,
		const std::string& data,
		CmdCallback callback )
{
	CmdId id = next_cmd_id++;

	send_command( id , cmd , data );
	commands[id] = callback;

	return id;
}

void IksClient::send_command( CmdId id
			    , const std::string& cmd
			    , const std::string& data )
{
	std::ostringstream os;
	os << cmd << " " << id << " " << data << "\n";
	socket->write( os.str() );
}

void IksClient::remove_command(CmdId id)
{
	commands.erase(id);
}

void IksClient::handle_read_line( boost::asio::streambuf& buf )
{
	std::istream is( &buf );
	std::string status;
	std::string data;
	CmdId id;

	std::getline(is, status, ' ');
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
		i->second( Error::no_error , status , data );
	else
		/*XXX: log */;
}

void IksClient::handle_error( const boost::system::error_code& ec )
{
	std::string empty;
	for ( auto& i : commands ) 
		i.second( Error::connection_error , "" , empty );

	commands.clear();
}

IksClient::~IksClient()
{
	socket->close();
}
