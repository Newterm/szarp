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

#include <liblog.h>

#include "global_service.h"

ProtocolLocation::ProtocolLocation( const std::string& name , Protocol::ptr protocol , Connection* connection )
	: Location(name,connection)
{
	set_protocol( protocol );
}

ProtocolLocation::~ProtocolLocation()
{
	for( auto ikv=commands.begin() ; ikv!=commands.end() ; ++ikv )
		delete ikv->second;
}

void ProtocolLocation::set_protocol( Protocol::ptr new_protocol )
{
	if( new_protocol ) {
		protocol = new_protocol;
		conn_send_cmd = protocol->on_send_cmd(
			std::bind(&ProtocolLocation::send_cmd,this,p::_1) );
		conn_location_request = protocol->on_location_request(
			std::bind(&ProtocolLocation::request_location,this,p::_1) );
	} else {
		conn_send_cmd.disconnect(); 
		conn_location_request.disconnect(); 
	}
}

void ProtocolLocation::request_location( Location::ptr loc )
{
	/** Check if requested location is protocol location */
	auto plp = std::dynamic_pointer_cast<ProtocolLocation>(loc);

	if( plp ) {
		/** If it is protocol location only swap protocols */
		set_protocol( plp->protocol );
		return;
	}

	/** Otherwise location is not protocol location so normal request */
	emit_request_location( loc );
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
		sz_log(0, "Got error from client (no. %d): %s" , cmd_id , data.c_str());
		erase_cmd( cmd_id );
	} else if( cmd_name == "r" || cmd_name == "k"  ) {
		if( !commands.count(cmd_id) ) {
			send_fail( cmd_id , ErrorCodes::invalid_id );
			return;
		}

		/** Response to command from client */
		commands[cmd_id]->new_line( data );

		if( cmd_name == "k" )
			erase_cmd( cmd_id ); /** proper command end */
	} else {
		if( commands.count(cmd_id) ) {
			send_fail( ErrorCodes::id_used );
			return;
		}

		/** New command from client */
		auto cmd = protocol->cmd_from_tag( cmd_name );

		if( !cmd ) {
			send_fail( cmd_id , ErrorCodes::unknown_command );
			return;
		}

		new_cmd( cmd , cmd_name , cmd_id , Command::to_send(data) );
	}
}

void ProtocolLocation::new_cmd( Command* cmd , const std::string& tag , id_t id , const Command::to_send& in_data )
{
	Command::to_send out_data = cmd->send_str();

	if( out_data )
		write_line( str( format("%s %d %s") % tag % id % *out_data ) );

	if( !cmd->single_shot() ) {
		if( !commands.count(id) ) {
			commands[id] = cmd;
			cmd->set_id( id );
			cmd->on_response( 
					std::bind(&ProtocolLocation::send_response,this,p::_1,p::_2,p::_3) );
		} else {
			/** This should never happen */
			sz_log(0, "Invalid id generated");
			return;
		}
	}

	if( in_data )
		cmd->new_line( *in_data );

	if( cmd->single_shot() )
		delete cmd;
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
	if( !protocol ) {
		sz_log(0, "Tried to send command without protocol set.");
		return;
	}

	auto tag = protocol->tag_from_cmd(cmd);

	if( tag.empty() ) {
		sz_log(0, "Tried to send command not implemented in this protocol.");
		return;
	}

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
				shared_from_this(), cmd) );
}

void ProtocolLocation::send_fail( ErrorCodes code , const std::string& msg )
{
	send_fail( 0 , code , msg );
}

void ProtocolLocation::send_fail( id_t id , ErrorCodes code , const std::string& msg )
{
	if( msg.empty() )
		write_line( str( format("e %u %u") % (unsigned)id % (unsigned)code ) );
	else
		write_line( str( format("e %u %u \"%s\"") % (unsigned)id % (unsigned)code % msg ) );
}

