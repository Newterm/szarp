#include "tcp_server.h"

#include <functional>
#include <utility>
#include <iostream>

#include <boost/algorithm/string.hpp>

#include <liblog.h>

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

	sz_log(1, "TcpServer error: %s", error.message().c_str());
	return true;
}

void TcpServer::do_accept()
{
	auto tc = std::make_shared<TcpConnection>( acceptor_.get_io_service() );
	tc->on_disconnect( bind(&TcpServer::do_disconnect,this,placeholders::_1) );
	clients.insert( tc );

    acceptor_.async_accept(tc->get_socket(),std::bind(&TcpServer::handle_accept,this,tc.get(),placeholders::_1) );
}

void TcpServer::do_disconnect( Connection* con )
{
	for( auto itr=clients.begin() ; itr!=clients.end() ; ++itr )
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

	try {
		emit_line_received( line );
	} catch(const std::exception& e ) {
		// we can catch here as boost signals and slots are in-place function calls by design
		sz_log(1, "Exception occurred during emit_line_received: %s" , e.what());
	}

	do_read_line();
}

void TcpConnection::do_write_line( const std::string& line )
{
	outbox.emplace_back( line.back() == '\n' ? line : line + '\n' );

	/** If there is no pending line start sending this one */
	if( sendbox.empty() )
		schedule_next_line();
}

void TcpConnection::schedule_next_line()
{
	std::vector<ba::const_buffer> bufs;

	// sendbox should be empty here
	// assert( sendbox.empty() );
	sendbox.swap( outbox );

	for( auto& line : sendbox )
	{
		bufs.push_back( ba::buffer( line ) );
	}

	ba::async_write(socket_,
		bufs,
		bind(&TcpConnection::handle_write, shared_from_this(), placeholders::_1));
}

void TcpConnection::handle_write(const bs::error_code& error)
{
	if( handle_error(error) )
		return;

	sendbox.clear();

	/** Continue sending */
	if( !outbox.empty() )
		schedule_next_line();
}

void TcpConnection::do_close()
{
	socket_.close();
}

