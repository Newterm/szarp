#include "location.h"

namespace p  = std::placeholders;

Location::Location( Connection* conn )
	: connection(conn)
{
	init_connection();
}

Location::Location( Location& loc )
	: connection(loc.connection)
{
	loc.sig_conn.disconnect();
	init_connection();
}

void Location::init_connection()
{
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

