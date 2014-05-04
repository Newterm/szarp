#ifndef __LINE_CLIENT_H__
#define __LINE_CLIENT_H__

#include <functional>

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include "connection.h"

#include "utils/signals.h"

class TcpClient : public Connection {
public:
	TcpClient( boost::asio::io_service& io_service, boost::asio::ip::tcp::resolver::iterator endpoint_iterator );

	virtual void write_line( const std::string& line )
	{
		io_service_.post(std::bind(&TcpClient::do_write_line, this, line));
	}

	void close()
	{
		io_service_.post(std::bind(&TcpClient::do_close, this));
	}

	connection on_connected( const sig_connection_slot& slot )
	{	return emit_connected.connect( slot ); }

private:
	bool handle_error( const boost::system::error_code& error );

	void handle_connect(const boost::system::error_code& error);
	void handle_read_line(const boost::system::error_code& error, size_t bytes );
	void handle_write(const boost::system::error_code& error);

	void do_read_line();
	void do_write_line( const std::string& line );
	void do_close();

private:
	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::socket socket_;

	boost::asio::streambuf buffer;

	sig_connection emit_connected;
};

#endif /* __LINE_CLIENT_H__ */

