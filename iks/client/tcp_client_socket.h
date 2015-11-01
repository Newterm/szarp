#ifndef IKS_TCP_SOCKET
#define IKS_TCP_SOCKET
#include "config.h"

#include <functional>
#include <boost/asio.hpp>

class TcpClientSocket : public std::enable_shared_from_this<TcpClientSocket>
{
public:

	class Handler {
	public:
		virtual void handle_read_line( boost::asio::streambuf& line ) = 0;
		virtual void handle_error( const boost::system::error_code& ec ) = 0;
		virtual ~Handler();
	};

private:
	boost::asio::ip::tcp::resolver resolver;
	boost::asio::ip::tcp::socket socket;

	std::string address;
	std::string port;

	boost::asio::streambuf i_buf;

	std::vector< std::string > o_buf_cur, o_buf_nxt;

	Handler& handler;

	void handle_error(const boost::system::error_code& ec );

	void do_read();

	void do_write();

public:
	TcpClientSocket(boost::asio::io_service& io, const std::string& address , const std::string& port , Handler& handler );

	void connect();

	void write(const std::string& string);

	void restart();

	void close();
};

#endif
