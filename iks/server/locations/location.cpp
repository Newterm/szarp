#include "location.h"

namespace p  = std::placeholders;

Location::Location( Connection* conn )
	: connection(conn)
{
	init_connection();
}

void Location::swap_connection( Location& loc )
{
	loc.sig_conn.disconnect();
	sig_conn.disconnect();

	std::swap( connection , loc.connection );

	loc.init_connection();
	init_connection();
}

void Location::init_connection()
{
	if( !connection )
		return;

	sig_conn = connection->on_line_received(
		std::bind(&Location::parse_line,this,std::placeholders::_1) );
}

Location::~Location()
{
}

void Location::write_line( const std::string& line )
{
	if( connection )
		connection->write_line( line );
}

