#include "protocol_location.h"

#include <random>
#include <iostream>

#if GCC_VERSION < 40600
#include <cstdlib>
#endif

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range.hpp> 

namespace ba = boost::algorithm;
namespace p  = std::placeholders;

using boost::format;

#include "global_service.h"

ProtocolLocation::ProtocolLocation( Connection* connection , Protocol* protocol )
	: Location(connection) , protocol(protocol)
{
}

ProtocolLocation::~ProtocolLocation()
{
	for( auto ikv=commands.begin() ; ikv!=commands.end() ; ++ikv )
		delete ikv->second;
}

void ProtocolLocation::set_protocol( Protocol* new_protocol )
{
	protocol = new_protocol;
}

void ProtocolLocation::parse_line( const std::string& line )
{
	if( !protocol ) {
		send_fail( ErrorCodes::protocol_not_set );
		return;
	}

	auto gap1 = ba::find_token( line, ba::is_any_of(" \t"), ba::token_compress_on );

	std::string cmd_name( line.begin() , gap1.begin() );

	auto remaining_line = boost::make_iterator_range( gap1.end() , line.end() );
	auto gap2 = ba::find_token( remaining_line , ba::is_any_of(" \t"), ba::token_compress_on );

	std::string cmd_id_str( gap1.end() , gap2.begin() );

	if( cmd_name.empty() || cmd_id_str.empty() ) {
		send_fail( ErrorCodes::ill_formed );
		return;
	}

	id_t cmd_id; 
	try {
		cmd_id = boost::lexical_cast<id_t>( cmd_id_str );
	} catch( const boost::bad_lexical_cast &) {
		send_fail( ErrorCodes::invalid_id );
		return;
	}

	std::string data( gap2.end() , line.end() );

	if( cmd_name == "e" ) {
		// TODO: log error or sth
		std::cerr << "Got error from client" << std::endl;
		erase_cmd( cmd_id );
	} else if( cmd_name == "k" ) {
		erase_cmd( cmd_id ); /** proper command end */
	} else if( cmd_name == "r" ) {
		if( !commands.count(cmd_id) ) {
			send_fail( ErrorCodes::invalid_id );
			return;
		}

		/** Response to command from client */
		commands[cmd_id]->new_line( data );
	} else {
		if( commands.count(cmd_id) ) {
			send_fail( ErrorCodes::id_used );
			return;
		}

		/** New command from client */
		auto cmd = protocol->cmd_from_tag( cmd_name );

		if( !cmd ) {
			send_fail( ErrorCodes::unknown_command );
			return;
		}

		new_cmd( cmd , cmd_name , cmd_id );
		cmd->new_line( data );
	}
}

void ProtocolLocation::new_cmd( Command* cmd , const std::string& tag , id_t id )
{
	Command::to_send data = cmd->send_str();

	if( data )
		write_line( str( format("%s %d %s") % tag % id % *data ) );

	if( !cmd->single_shot() && !commands.count(id) ) {
		commands[id] = cmd;
		cmd->set_id( id );
		cmd->on_response( 
				std::bind(&ProtocolLocation::send_response,this,p::_1,p::_2,p::_3) );
	} else {
		if( !cmd->single_shot() )
			std::cerr << "Invalid id generated" << std::endl;
		delete cmd;
	}
}

void ProtocolLocation::erase_cmd( Command* cmd )
{
	erase_cmd( cmd->get_id() );
}

void ProtocolLocation::erase_cmd( id_t id )
{
	auto itr = commands.find( id );
	if( itr != commands.end() ) {
		delete itr->second;
		commands.erase( itr );
	}
}

id_t ProtocolLocation::generate_id()
{
	/* TODO: Better way to generate unique id in future
	 * (17/03/2014 16:48, jkotur) */
	using std::max;

	auto maxid = max(8096,(int)commands.size()*4);
#if GCC_VERSION >= 40600
	std::uniform_int_distribution<id_t> dist(1,maxid);
#endif
	id_t id;

	do {
#if GCC_VERSION >= 40600
		id = dist(rnd);
#else
		id = rand() % maxid;
#endif
	} while( commands.count(id) );

	return id;
}

void ProtocolLocation::send_cmd( Command* cmd )
{
	/* TODO: Report errors (04/05/2014 20:03, jkotur) */

	if( !protocol )
		return;

	auto tag = protocol->tag_from_cmd(cmd);

	if( tag.empty() )
		return;

	new_cmd( cmd , tag , cmd->single_shot() ? 0 : generate_id() );
}

void ProtocolLocation::send_response(
		Command::ResponseType type ,
		const std::string& data ,
		Command* cmd )
{
	std::string cmd_name;

	if( type == Command::ResponseType::OK )
		cmd_name = "k";
	else if( type == Command::ResponseType::ERROR )
		cmd_name = "e";
	else if( type == Command::ResponseType::RESPONSE )
		cmd_name = "r";

	if( !cmd_name.empty() )
		write_line( str( format("%s %d %s") % cmd_name % cmd->get_id() % data ) );

	/**
	 * Remove command handler if sending accept or error.
	 * This assumes no other message will come.
	 *
	 * This is done asynchronously to make sure that its not called
	 * with Command method in stack.
	 */
	if( type != Command::ResponseType::RESPONSE )
		GlobalService::get_service().post(
			std::bind(
				static_cast<void(ProtocolLocation::*)(Command*)>(
					&ProtocolLocation::erase_cmd),
				this,cmd) );
}

void ProtocolLocation::send_fail( ErrorCodes code )
{
	write_line( str( format("e 0 %u") % (unsigned)code ) );
}

