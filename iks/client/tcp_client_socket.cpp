#include "tcp_client_socket.h"

namespace ba = boost::asio;
namespace bs = boost::system;

void TcpClientSocket::handle_error(const boost::system::error_code& ec )
{
	if ( ec == ba::error::operation_aborted)
		return;

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

	assert( o_buf_cur.size() );
	
	auto self = shared_from_this();
	
	std::vector<boost::asio::const_buffer> buf_v;
	for ( auto& s : o_buf_cur )
		buf_v.emplace_back( boost::asio::buffer( s ) );

	ba::async_write( socket , buf_v , [self] ( const bs::error_code& ec , const std::size_t& ) {
		if ( ec )
		{
			self->handle_error( ec );
			return;
		}

		self->o_buf_cur.clear();

		if ( self->o_buf_nxt.size() )
		{
			std::swap( self->o_buf_cur , self->o_buf_nxt );
			self->do_write();
		}
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
	connect();
}

void TcpClientSocket::connect()
{
	o_buf_cur.clear();
	o_buf_nxt.clear();

	if ( socket.is_open() )
		socket.close();

	namespace bip = boost::asio::ip;

	auto self = shared_from_this();
	resolver.async_resolve ( bip::tcp::resolver::query( address , port ) 
			       , [self] ( const bs::error_code& ec , bip::tcp::resolver::iterator i ) {
		if ( ec )
		{
			self->handle_error( ec );
			return;
		}

		self->socket.async_connect( *i , [self] ( const bs::error_code& ec ) {
			if ( ec )
			{
				self->handle_error( ec );
				return;
			}

			self->do_write();
			self->do_read();
		});

	});
}

void TcpClientSocket::write(const std::string& string)
{
	o_buf_nxt.push_back(string);

	if ( o_buf_cur.size() == 0 ) {
		std::swap( o_buf_cur , o_buf_nxt );
		do_write();
	}
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
