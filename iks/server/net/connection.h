#ifndef __NET_CONNECTION_H__
#define __NET_CONNECTION_H__

#include "utils/signals.h"

class Connection;

typedef boost::signals2::signal<void (Connection*)> sig_connection;
typedef sig_connection::slot_type sig_connection_slot;

class Connection {
public:
	virtual ~Connection() {}

	virtual void write_line( const std::string& line ) = 0;

	connection on_line_received( const sig_line_slot& slot )
	{	return emit_line_received.connect( slot ); }
	connection on_disconnect( const sig_connection_slot& slot )
	{	return emit_disconnected.connect( slot ); }

protected:
	sig_line       emit_line_received;
	sig_connection emit_disconnected;

};

#endif /* end of include guard: __NET_CONNECTION_H__ */

