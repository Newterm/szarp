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

TcpClient::TcpClient( ba::io_service& io_service, tcp::resolver::iterator endpoint_iterator )
		: io_service_(io_service)
		, socket_(io_service)
{
	tcp::endpoint endpoint = *endpoint_iterator;
	socket_.async_connect(endpoint, bind(&TcpClient::handle_connect, this, placeholders::_1));
}

bool TcpClient::handle_error( const bs::error_code& error )
{
	if( !error ) 
		return false;

	std::cerr << "---      " << "TcpClient disconnected ("
	          << error.message() << ")" << std::endl;

	return true;
}

void TcpClient::handle_connect(const bs::error_code& error)
{
	if( handle_error(error) )
		return;

	const auto& e = socket_.remote_endpoint();
	std::cout << "+++      Connected to " << e.address().to_string() << ":" << e.port() << std::endl;

	emit_connected( this );
	do_read_line();
}

void TcpClient::do_read_line()
{
	ba::async_read_until(socket_,
			buffer,
			'\n',
			bind(&TcpClient::handle_read_line, this, placeholders::_1, placeholders::_2 ));
}

void TcpClient::handle_read_line( const bs::error_code& error, size_t bytes )
{
	if( handle_error(error) )
		return;

	ba::streambuf::const_buffers_type bufs = buffer.data();
	std::string line(
		ba::buffers_begin(bufs),
		ba::buffers_begin(bufs) + bytes - 1 );
	buffer.consume( bytes );

	balgo::trim( line );

	std::cout << ">>>      " << line << std::endl;

	emit_line_received( line );

	do_read_line();
}

void TcpClient::do_write_line( const std::string& line )
{
	ba::streambuf sb;
	std::ostream os(&sb);

	os << line ;
	if( line[line.size()-1] != '\n' )
		os << '\n';

	ba::async_write(socket_,
		sb,
		bind(&TcpClient::handle_write, this, placeholders::_1));

	std::cout << "<<<      " << line << std::endl;
}

void TcpClient::handle_write(const bs::error_code& error)
{
	if( handle_error(error) )
		return;
}

void TcpClient::do_close()
{
	socket_.close();
}

