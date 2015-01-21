#include "handler.h"

#include <random>
#include <functional>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range.hpp> 

#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>

namespace ba = boost::algorithm;
namespace p  = std::placeholders;

using boost::format;

#include "command_factory.h"
#include "global_service.h"
#include "probes_srv.h"
#include "thread_pool.h"

ClientHandler::ClientHandler( SzbCache& szbcache , Connection* conn , ThreadPool& tpool )
	: m_szbcache(szbcache) , connection(conn) , thread_pool(tpool)
{
	conn_line = connection->on_line_received(
		std::bind(&ClientHandler::parse_line,this,p::_1) );
}

ClientHandler::~ClientHandler()
{
	for( auto& kv : commands )
		delete kv.second;
}


void ClientHandler::parse_line( const std::string& line )
{
	auto gap1 = ba::find_token( line, ba::is_any_of(" \t"), ba::token_compress_on );

	std::string cmd_name( line.begin() , gap1.begin() );

	if( cmd_name.empty() ) {
		send_fail( ErrorCodes::bad_request );
		return;
	}

	std::string data( gap1.end() , line.end() );

	/** New command from client */
	auto cmd = CmdFactory::cmd_from_tag( cmd_name , *this );

	if( !cmd ) {
		send_fail( ErrorCodes::bad_request );
		return;
	}

	cmd->set_data(data);

	thread_pool.run_task( boost::bind(&ClientHandler::run_cmd, this, cmd) );
}

void ClientHandler::run_cmd( CommandHandler* cmd )
{
	sz_log(10, "ClientHandler::new_cmd start %s", cmd->get_tag());

	cmd->on_response( std::bind(&ClientHandler::send_response, this, p::_1, p::_2, p::_3) );

	cmd->run();

	sz_log(10, "ClientHandler::new_cmd end %s", cmd->get_tag());
	
	delete cmd;
}

void ClientHandler::erase_cmd( CommandHandler* cmd )
{
	sz_log(10, "ClientHandler::erase_cmd %d", cmd->id);
	erase_cmd( cmd->id );
}

void ClientHandler::erase_cmd( id_t id )
{
	auto itr = commands.find( id );
	if( itr != commands.end() ) {
		delete itr->second;
		commands.erase( itr );
	}
}

id_t ClientHandler::generate_id()
{
	/* TODO: Better way to generate unique id in future
	 * (17/03/2014 16:48, jkotur) */
	using std::max;

	std::uniform_int_distribution<id_t> dist(1,max(8096,(int)commands.size()*4));
	id_t id;

	do {
		id = dist(rnd);
	} while( commands.count(id) );

	return id;
}

void ClientHandler::send_response(
		CommandHandler::ResponseType type ,
		const std::string& data ,
		CommandHandler* cmd )
{
	sz_log(10, "ClientHandler::send_response data: %s", data.c_str());
	connection->write_line(data);

	/**
	 * Remove command handler if sending accept or error.
	 * This assumes no other message will come.
	 *
	 * This is done asynchronously to make sure that its not called
	 * with CommandHandler method in stack.
	 */
	/*
	if( type != CommandHandler::ResponseType::RESPONSE ) {
		sz_log(8, "ClientHandler::send_response not response, erase cmd");

		GlobalService::get_service().post(
			std::bind(
				static_cast<void(ClientHandler::*)(CommandHandler*)>(
					&ClientHandler::erase_cmd),
				this, cmd) );
	}
	*/
}

void ClientHandler::send_fail( ErrorCodes code )
{
	connection->write_line( str( format("e 0 %u") % (unsigned)code ) );
}

SzbCache& ClientHandler::get_szbcache() { return m_szbcache; };

int ClientHandler::get_native_fd() { return connection->get_native_fd(); };

