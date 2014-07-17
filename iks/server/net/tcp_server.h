#ifndef __LINE_TCP_SERVER_H__
#define __LINE_TCP_SERVER_H__

#include <unordered_set>
#include <functional>
#include <queue>

#include <boost/asio.hpp>

#include "connection.h"

#include "utils/signals.h"

class TcpConnection;

class TcpServer {
public:
	TcpServer( boost::asio::io_service& io_service, const boost::asio::ip::tcp::endpoint& endpoint);

	slot_connection on_connected( const sig_connection_slot& slot )
	{	return emit_connected.connect( slot ); }

	slot_connection on_disconnected( const sig_connection_slot& slot )
	{	return emit_disconnected.connect( slot ); }

private:
	bool handle_error( const boost::system::error_code& error );

	void handle_accept(
			TcpConnection* connection ,
			const boost::system::error_code& error );

	void do_accept();

	void do_disconnect( Connection* connection );

	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::socket socket_;

	std::unordered_set<std::shared_ptr<Connection>> clients;

	sig_connection emit_connected;
	sig_connection emit_disconnected;
};

class TcpConnection : public Connection , public std::enable_shared_from_this<TcpConnection> {
	friend class TcpServer;

public:
	TcpConnection( boost::asio::io_service& service );

	virtual void close()
	{	do_close(); }

	void write_line( const std::string& line )
	{	do_write_line( line ); }

private:
	void start();

	bool handle_error( const boost::system::error_code& error );

	void handle_read_line(const boost::system::error_code& error, size_t bytes );
	void handle_write(const boost::system::error_code& error);

	void do_write_line( const std::string& line );
	void do_read_line();
	void do_close();

	boost::asio::ip::tcp::socket& get_socket()
	{	return socket_; }

	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf read_buffer;

	std::queue<std::string> lines;
};

#endif /* __LINE_SERVER_H__ */

