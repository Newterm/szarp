#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "iks_location_connnction.h"

namespace boost::property_tree = bp;

void IksLocationConnection::connect() 
{
	auto self = shared_from_this();

	client.send_command( "list_remotes" , ""
					   , [self] ( IksConnection::Error error , const std::string& status , std::string& data ) {
		if ( error )
			return IksConnection::cmd_done;

		if ( status != "k" ) {
			return IksConnection::cmd_done;
		}

		std::string target;
		try{
			std::stringstream ss( data );
			bp::ptree json;
			bp::json_parser::read_json( ss , json );

			for(auto itr = json.begin() ; itr!=json.end() ; ++itr )
				if ( itr->second.get<std::string>("type") == type
				   && itr->second.get<std::string>("name") == name )
					target = itr->first;
		
			if( target.empty() ) {
				self->on_location_connection_error(location_not_present);
				return IksConnection::cmd_done;
			}

		} catch( const bp::ptree_error& e ) {
			self->on_location_connection_error(invalid_response);
			return IksConnection::cmd_done;
		}

		self->client.send_command( "connect" , target
								 , [self] ( IksConnection::Error error
										  , const std::string& status
										  , std::string& data ) {

				if ( error )
					return IksConnection::cmd_done;

				if ( status != "k" ) {
					self->on_location_connection_error(failed_to_connect);
					return IksConnection::cmd_done;
				}

				self->on_connected_to_location();
		
		});

	});
}

IksLocationConnection::IksLocationConnection( boost::asio::io_service& io
											, const std::string& location
											, const std::string& type
											, const std::string& server
											, const std::string& port,
											, IksCmdReceiver *receiver)
											: location( location )
											, type( type )
											, client ( server , port , this)
											, receiver( receiver )
{ 

}

IksConnection::CmdId IksLocationConnection::send_command( const std::string& cmd
													, const std::string& data,
													, CmdCallback callback )
{
	client.send_command( cmd , data , callback );
}

void IksConnection::send_command( IksConnection::CmdId id
							, const std::string& cmd
							, const std::string& data)
{
	client.send_command( id , cmd, data );
}

void IksLocationConnection::on_cmd(const std::string& tag, IksConnection::CmdId id, std::string& data)
{
	receiver->on_cmd( tag , id , data );
}

void IksLocationConnection::on_connected()
{
	connect();
}

void IksLocationConnection::on_connection_error(const boost::system::error_code& ec) 
{
	receiver->on_connection_error(ec);
}
