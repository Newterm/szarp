#include "tcp_client.h"

#include <functional>
#include <iostream>

using std::bind;
namespace placeholders = std::placeholders;

#include <boost/algorithm/string.hpp>

namespace ba = boost::asio;
namespace bs = boost::system;
namespace balgo = boost::algorithm;
using boost::asio::ip::tcp;

#include <liblog.h>

TcpClient::TcpClient( ba::io_service& io_service, tcp::resolver::iterator endpoint_iterator )
		: io_service_(io_service)
		, socket_(io_service)
{
	handler = std::make_shared<details::AsioHandler>(*this);

	tcp::endpoint endpoint = *endpoint_iterator;
	socket_.async_connect(endpoint, bind(&details::AsioHandler::handle_connect, handler, placeholders::_1));
}

TcpClient::~TcpClient()
{
	if( handler->is_valid() ) {
		emit_disconnected( this );
		handler->invalidate();
	}
}

void TcpClient::do_write_line( const std::string& line )
{
	lines.emplace( line.back() == '\n' ? line : line + '\n' );

	ba::async_write(socket_,
		ba::buffer( lines.back() ),
		bind(&details::AsioHandler::handle_write, handler, placeholders::_1));

	sz_log(9, "<<<      %s", line.c_str() );
}

void TcpClient::do_close()
{
	socket_.close();
}

namespace details {

bool AsioHandler::handle_error( const bs::error_code& error )
{
	if( !error ) 
		return false;

	sz_log(3, "---      TcpClient disconnected (%s)", error.message().c_str() );
 
	if( is_valid() ) {
		client.emit_disconnected( &client );
		invalidate();
	}

	return true;
}

void AsioHandler::handle_connect(const bs::error_code& error)
{
	if( handle_error(error) || !is_valid() )
		return;

	const auto& e = client.socket_.remote_endpoint();
	sz_log(3, "+++      Connected to %s:%d", e.address().to_string().c_str(),  e.port());

	client.emit_connected( &client );
	do_read_line();
}

void AsioHandler::do_read_line()
{
	ba::async_read_until(client.socket_,
			client.read_buffer,
			'\n',
			bind(&AsioHandler::handle_read_line, shared_from_this(), placeholders::_1, placeholders::_2 ));
}

void AsioHandler::handle_read_line( const bs::error_code& error, size_t bytes )
{
	if( handle_error(error) || !is_valid() )
		return;

	ba::streambuf::const_buffers_type bufs = client.read_buffer.data();
	std::string line(
		ba::buffers_begin(bufs),
		ba::buffers_begin(bufs) + bytes - 1 );
	client.read_buffer.consume( bytes );

	balgo::trim( line );

	sz_log(9, ">>>      %s", line.c_str());

	client.emit_line_received( line );

	do_read_line();
}

void AsioHandler::handle_write(const bs::error_code& error)
{
	if( handle_error(error) || !is_valid() )
		return;

	/** This line was send */
	client.lines.pop();
}

} /** namespace details */

