#include "tcp_server.h"

#include <functional>
#include <utility>
#include <iostream>

#include <boost/algorithm/string.hpp>

#include <liblog.h>

namespace ba = boost::asio;
namespace bs = boost::system;
namespace balgo = boost::algorithm;
namespace placeholders = std::placeholders;

using boost::asio::ip::tcp;
using std::bind;


TcpServer::TcpServer( ba::io_service& io_service, const tcp::endpoint& endpoint)
    : io_service_(io_service),
	acceptor_(io_service, endpoint),
	socket_(io_service)
{
	do_accept();
}

bool TcpServer::handle_error( const bs::error_code& error )
{
	if( !error ) 
		return false;

	sz_log(0, "TcpServer error: %s", error.message().c_str());
	return true;
}

void TcpServer::do_accept()
{
	auto tc = std::make_shared<TcpConnection>( acceptor_.get_io_service() );
	tc->on_disconnect( bind(&TcpServer::do_disconnect, this, placeholders::_1) );
	clients.insert( tc );

	acceptor_.async_accept(tc->get_socket(),std::bind(&TcpServer::handle_accept,this,tc.get(),placeholders::_1) );
}

void TcpServer::do_disconnect( Connection* con )
{
	for( auto itr=clients.begin() ; itr != clients.end() ; ++itr )
		if( itr->get() == con ) {
			clients.erase( itr );
			break;
		}

	emit_disconnected( con );
}

void TcpServer::handle_accept(
		TcpConnection* connection ,
		const bs::error_code& error )
{
	if( handle_error(error) )
		return;

	connection->start();

	emit_connected( connection );

	do_accept();
}

TcpConnection::TcpConnection( ba::io_service& service )
	: socket_(service)
{
}

void TcpConnection::start()
{
	sz_log(3, "   +++   New client %s",
		socket_.remote_endpoint().address().to_string().c_str());

	do_read_line();
}

bool TcpConnection::handle_error( const bs::error_code& error )
{
	if( !error )
		return false;

	sz_log(3, "   ---   Client disconnected (%s)", error.message().c_str());

	emit_disconnected( this );
	return true;
}

void TcpConnection::do_read_line()
{
	ba::async_read_until(socket_,
			read_buffer,
			'\n',
			bind(&TcpConnection::handle_read_line, shared_from_this(), placeholders::_1, placeholders::_2 ));
}

void TcpConnection::handle_read_line(const bs::error_code& error, size_t bytes )
{
	if( handle_error(error) )
		return;

	ba::streambuf::const_buffers_type bufs = read_buffer.data();
	std::string line(
		ba::buffers_begin(bufs),
		ba::buffers_begin(bufs) + bytes - 1 );
	read_buffer.consume( bytes );

	balgo::trim( line );

	sz_log(9, "   <<<   %s", line.c_str());

	emit_line_received( line );

	do_read_line();
}

void TcpConnection::do_write_line( const std::string& line )
{
	lines.emplace(line);

	ba::async_write(socket_,
		ba::buffer( lines.back() ),
		bind(&TcpConnection::handle_write, shared_from_this(), placeholders::_1));

	sz_log(9, "   >>>   %s", line.c_str() );
}

void TcpConnection::handle_write(const bs::error_code& error)
{
	if( handle_error(error) )
		return;

	/** This line was send */
	lines.pop();
}

void TcpConnection::do_close()
{
	socket_.close();
}

int TcpConnection::get_native_fd()
{
	boost::system::error_code ec;
	if (!socket_.native_non_blocking())
		socket_.native_non_blocking(true, ec); 

	if (ec)
		return -1;
	return socket_.native_handle();
}
