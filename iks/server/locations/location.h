#ifndef __CONNECTIONS_CONNECTION_H__
#define __CONNECTIONS_CONNECTION_H__

#include <functional>

#include "net/connection.h"

#include "utils/signals.h"

class Location {
public:
	Location( Connection* conn );
	virtual ~Location();

protected:
	virtual void parse_line( const std::string& line ) =0;

	void write_line( const std::string& line );

private:
	Connection* connection;

	boost::signals2::scoped_connection sig_conn;
};

#endif /* end of include guard: __CONNECTIONS_CONNECTION_H__ */

