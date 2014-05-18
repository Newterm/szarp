#include "tcp_server.h"

#include <functional>
#include <utility>
#include <iostream>

#include <boost/algorithm/string.hpp>

namespace ba = boost::asio;
namespace bs = boost::system;
namespace balgo = boost::algorithm;
using boost::asio::ip::tcp;

using std::bind;
namespace placeholders = std::placeholders;

TcpServer::TcpServer( ba::io_service& io_service, const tcp::endpoint& endpoint)
    : io_service_(io_service)
	, acceptor_(io_service, endpoint)
    , socket_(io_service)
{
	do_accept();
}

bool TcpServer::handle_error( const bs::error_code& error )
{
	if( !error ) 
		return false;

	std::cerr << "TcpServer error: " << error.message() << std::endl;
	return true;
}

void TcpServer::do_accept()
{
	auto tc = new TcpConnection( acceptor_.get_io_service() );
	tc->on_disconnect( bind(&TcpServer::do_disconnect,this,placeholders::_1) );
	clients.insert( tc );

    acceptor_.async_accept(tc->get_socket(),std::bind(&TcpServer::handle_accept,this,tc,placeholders::_1) );
}

void TcpServer::do_disconnect( Connection* con )
{
	clients.erase( con );
	emit_disconnected( con );
	delete con;
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
	std::cout << "   +++   New client " << socket_.remote_endpoint().address().to_string() << std::endl;

	do_read_line();
}

bool TcpConnection::handle_error( const bs::error_code& error )
{
	if( !error )
		return false;

	std::cout << "   ---   Client disconnected "
			  << " (" << error.message() << ")" << std::endl;

	emit_disconnected( this );
	return true;
}

void TcpConnection::do_read_line()
{
	ba::async_read_until(socket_,
			buffer,
			'\n',
			bind(&TcpConnection::handle_read_line, this, placeholders::_1, placeholders::_2 ));
}

void TcpConnection::handle_read_line(const bs::error_code& error, size_t bytes )
{
	if( handle_error(error) )
		return;

	ba::streambuf::const_buffers_type bufs = buffer.data();
	std::string line(
		ba::buffers_begin(bufs),
		ba::buffers_begin(bufs) + bytes - 1 );
	buffer.consume( bytes );

	balgo::trim( line );

	std::cout << "   <<<   " << line << std::endl;

	emit_line_received( line );

	do_read_line();
}

void TcpConnection::do_write_line( const std::string& line )
{
	ba::streambuf sb;
	std::ostream os(&sb);

	os << line ;
	if( line[line.size()-1] != '\n' )
		os << '\n';

	ba::async_write(socket_,
		sb,
		bind(&TcpConnection::handle_write, this, placeholders::_1));

	std::cout << "   >>>   " << line << std::endl;
}

void TcpConnection::handle_write(const bs::error_code& error)
{
	if( handle_error(error) )
		return;
}

void TcpConnection::do_close()
{
}

