#ifndef __LOCATIONS_COMMAND_H__
#define __LOCATIONS_COMMAND_H__

#include <string>
#include <functional>

#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>

#include "error_codes.h"
#include "utils/signals.h"

typedef unsigned int id_t;

class Command {
public:
	enum class ResponseType { RESPONSE , OK , ERROR , QUIET_END };

	typedef boost::signals2::signal<void (ResponseType,const std::string&,Command*)> sig_response;
	typedef std::function<void (const std::string& data)> line_handler;
	typedef boost::optional<std::string> to_send;

	virtual ~Command() {}

	void set_id( id_t _id ) {	id = _id; }
	id_t get_id() { return id; }

	virtual to_send send_str()
	{	return to_send(); }

	virtual bool single_shot()
	{	return false; }

	void new_line( const std::string& data )
	{
		if( !next_handler )
			return;

		auto hnd = next_handler;
		/** Default handler, could be changed in hnd function */
		next_handler = std::bind(&Command::default_handler,this,std::placeholders::_1);
		hnd( data );
	}

	slot_connection on_response( const sig_response::slot_type& slot )
	{	return emit_response.connect( slot ); }

protected:
	void set_next( const line_handler& hnd )
	{	next_handler = hnd; }

	void apply( const std::string& data = "")
	{	emit_response( ResponseType::OK , data , this ); }

	void fail( ErrorCodes code , const std::string& msg = "" )
	{
		std::string data;
		if( msg.empty() )
			data = boost::lexical_cast<std::string>((error_code_t)code); 
		else 
			data = str( boost::format("%d \"%s\"") % (error_code_t)code % msg );
		emit_response(
			ResponseType::ERROR ,
			data ,
			this );
	}

	void response( const std::string& data = "" )
	{	emit_response( ResponseType::RESPONSE , data , this ); }

	void quiet_end()
	{	emit_response( ResponseType::QUIET_END , "" , this ); }

	void default_handler( const std::string& data )
	{	fail( ErrorCodes::unknown_command ); }

private:
	/* Needed by client handler to know what id send back.
	 * Don't know how to get rid of it (13/03/2014 20:15, jkotur) */
	id_t id;

	line_handler next_handler;
	sig_response emit_response;
};

typedef boost::signals2::signal<void (const Command&)> sig_cmd;
typedef sig_cmd::slot_type sig_cmd_slot;

typedef boost::signals2::signal<void (Command*)> sig_cmd_ptr;
typedef sig_cmd_ptr::slot_type sig_cmd_ptr_slot;

#endif /* end of include guard: __LOCATIONS_COMMAND_H__ */

