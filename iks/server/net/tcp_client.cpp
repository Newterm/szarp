#include "tcp_client.h"

#include <functional>
#include <iostream>

#include <boost/algorithm/string.hpp>

namespace ba = boost::asio;
namespace bs = boost::system;

using boost::asio::ip::tcp;

#include <liblog.h>

TcpClient::TcpClient( ba::io_service& io_service, tcp::resolver::iterator endpoint_iterator )
		: io_service_(io_service)
		, socket_(io_service)
{
	handler = std::make_shared<details::AsioHandler>(*this);

	tcp::endpoint endpoint = *endpoint_iterator;
	socket_.async_connect(endpoint, [this](const bs::error_code& err){
				handler->handle_connect(err);
		  	} );
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
	outbox.emplace_back( line.back() == '\n' ? line : line + '\n' );

	sz_log(9, "<<<      %.*s", (int)outbox.back().size()-1, outbox.back().c_str() );

	if( sendbox.empty() )
		schedule_next_line();
}

void TcpClient::schedule_next_line()
{
	std::vector<ba::const_buffer> bufs;

	sendbox.swap( outbox );

	for( auto& line : sendbox )
	{
		bufs.push_back( ba::buffer( line ) );
	}

	ba::async_write(socket_,
		bufs,
		[this](const bs::error_code& err, std::size_t){
			handler->handle_write(err);
		} );
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
			[this](const bs::error_code& error, size_t bytes ){
				handle_read_line(error, bytes);
			} );
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

	boost::algorithm::trim( line );

	sz_log(9, ">>>      %s", line.c_str());

	try {
		client.emit_line_received( line );
	} catch( const std::exception& e ) {
		sz_log(9, "Exception occurred during emit_line_received: %s" , e.what() );
	}

	do_read_line();
}

void AsioHandler::handle_write(const bs::error_code& error)
{
	if( handle_error(error) || !is_valid() )
		return;

	/** This line was send */
	client.sendbox.clear();

	if( !client.outbox.empty() )
		client.schedule_next_line();
}

} /** namespace details */

