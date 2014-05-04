#include "location.h"

namespace p  = std::placeholders;

Location::Location( Connection* conn )
	: connection(conn)
{
	sig_conn = connection->on_line_received(
		std::bind(&Location::parse_line,this,std::placeholders::_1) );
}

Location::~Location()
{
}

void Location::write_line( const std::string& line )
{
	connection->write_line( line );
}

