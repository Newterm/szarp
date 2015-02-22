#ifndef __SERVER_COMMAND_H__
#define __SERVER_COMMAND_H__

#include <string>
#include <functional>

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include "error_codes.h"
#include "net/connection.h"
#include "utils/signals.h"
#include "liblog.h"

namespace p = std::placeholders;

typedef unsigned int id_t;

class ClientHandler;

class CommandHandler {
	friend class ClientHandler;

public:
	enum class ResponseType { RESPONSE , OK , ERROR };

	typedef boost::signals2::signal<void (ResponseType, const std::string&, CommandHandler*)> sig_response;
	typedef std::function<void (const std::string& data)> line_handler;

	virtual ~CommandHandler() {}

	void set_data(const std::string& data_)
	{	data = data_;	}

        virtual const char* get_tag() = 0;

	void run()
	{
		if( !next_handler )
			return;

		auto _hnd = next_handler;
		/** Default handler, could be changed in hnd function */
		next_handler = std::bind(&CommandHandler::default_handler, this, p::_1);
		sz_log(8, "CommandHandler::run before handler d: %s", data.c_str());
		_hnd( data );
		sz_log(8, "CommandHandler::run after handler");
	}

	slot_connection on_response( const sig_response::slot_type& slot )
	{	return emit_response.connect( slot ); }

protected:
	void set_id(id_t _id)
	{	id = _id; }

	void set_next( const line_handler& hnd )
	{	next_handler = hnd; }

	void apply( const std::string& data = "")
	{
		emit_response( ResponseType::OK , data , this ); 
	}

	void response( const std::string& data = "" )
	{	emit_response( ResponseType::RESPONSE , data , this ); }

	void fail( ErrorCodes code )
	{
		emit_response(
			ResponseType::ERROR ,
			boost::lexical_cast<std::string>((error_code_t)code) ,
			this );
	}

	void default_handler( const std::string& data )
	{	fail( ErrorCodes::bad_request ); }

	std::string data;
private:
	/* Needed by client handler to know what id send back.
	 * Don't know how to get rid of it (13/03/2014 20:15, jkotur) */
	id_t id;

	line_handler next_handler;
	sig_response emit_response;
};


#endif /* end of include guard: __SERVER_COMMAND_H__ */

