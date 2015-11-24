#include <boost/asio.hpp>

#include "tcp_client_socket.h"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace bae = boost::asio::error;

void TcpClientSocket::handle_error(const bs::error_code& ec )
{
	handler.handle_error( ec );
}

void TcpClientSocket::do_read()
{
	auto self = shared_from_this();
	ba::async_read_until( socket , i_buf , '\n'
						, [self] ( const bs::error_code& ec , std::size_t bytes_read ) {
		if ( ec ) {
			self->handle_error( ec );
			return;
		}

		self->handler.handle_read_line( self->i_buf );

		self->do_read();
	});

}

void TcpClientSocket::do_write()
{
	if ( !socket.is_open() )
		return;
	
	if ( o_buf_cur.size()  != 0)
		return;

	if ( o_buf_nxt.size()  == 0)
		return;

	std::swap( o_buf_cur , o_buf_nxt );

	auto self = shared_from_this();
	
	std::vector<ba::const_buffer> buf_v;
	for ( auto& s : o_buf_cur )
		buf_v.emplace_back( ba::buffer( s ) );

	ba::async_write( socket , buf_v , [self] ( const bs::error_code& ec , const std::size_t& ) {
		if ( ec )
		{
			self->handle_error( ec );
			return;
		}

		self->o_buf_cur.clear();

		self->do_write();
	});
}

TcpClientSocket::TcpClientSocket( boost::asio::io_service& io 
								, const std::string& address
								, const std::string& port
								, Handler& handler )
								: resolver( io )
								, socket( io )
								, address( address )
								, port( port )
								, handler( handler )
{
}

void TcpClientSocket::connect()
{
	o_buf_cur.clear();
	o_buf_nxt.clear();

	if ( socket.is_open() )
		socket.close();

	namespace bip = boost::asio::ip;

	auto self = shared_from_this();
	resolver.async_resolve ( bip::tcp::resolver::query( bip::tcp::v4() , address , port ) 
						   , [self] ( const bs::error_code& ec , bip::tcp::resolver::iterator i ) {
		if ( ec )
		{
			if ( ec != bae::operation_aborted )
				self->handle_error( ec );
			return;
		}

		self->resolver.cancel();

		self->socket.async_connect( *i , [self] ( const bs::error_code& ec ) {
			if ( ec )
			{
				self->handle_error( ec );
				return;
			}

			self->handler.handle_connected();

			self->do_write();
			self->do_read();
		});

	});
}

void TcpClientSocket::write(const std::string& string)
{
	o_buf_nxt.push_back(string);

	do_write();
}

void TcpClientSocket::restart()
{
	close();
	connect();
}

void TcpClientSocket::close() 
{
	resolver.cancel();

	if (socket.is_open()) 
		socket.close();
}
/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
