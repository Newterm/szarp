#ifndef __LINE_CLIENT_H__
#define __LINE_CLIENT_H__

#include <memory>
#include <functional>
#include <deque>

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include "connection.h"

#include "utils/signals.h"

namespace details { class AsioHandler; }

class TcpClient : public Connection {
	friend class details::AsioHandler;

public:
	TcpClient( boost::asio::io_service& io_service, boost::asio::ip::tcp::resolver::iterator endpoint_iterator );
	virtual ~TcpClient();

	virtual void write_line( const std::string& line )
	{
		io_service_.post([this, line](){ do_write_line(line); } );
	}

	void close()
	{
		io_service_.post([this](){ do_close(); } );
	}

	slot_connection on_connected( const sig_connection_slot& slot )
	{	return emit_connected.connect( slot ); }

	bool is_connected() const { return socket_.is_open(); }

private:
	void do_write_line( const std::string& line );

	void do_close();

	void schedule_next_line();

	std::shared_ptr<details::AsioHandler> handler;

	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::socket socket_;

	boost::asio::streambuf read_buffer;

	std::deque<std::string> outbox;
	std::deque<std::string> sendbox;

	sig_connection emit_connected;

};

namespace details {

class AsioHandler : public std::enable_shared_from_this<AsioHandler> {
public:
	AsioHandler( TcpClient& client ) : client(client) , valid(true) {}

	bool is_valid() { return valid; }
	void invalidate() { valid = false; }

	bool handle_error( const boost::system::error_code& error );

	void handle_connect(const boost::system::error_code& error);
	void handle_read_line(const boost::system::error_code& error, size_t bytes );
	void handle_write(const boost::system::error_code& error);

	void do_read_line();

	TcpClient& client;
	bool valid;
};

}

#endif /* __LINE_CLIENT_H__ */

