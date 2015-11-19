#ifndef IKS_CONNECTION_H
#define IKS_CONNECTION_H

#include <unordered_map>
#include <boost/signals2.hpp>

#include "tcp_client_socket.h"

class IksConnection : public std::enable_shared_from_this<IksConnection>, public TcpClientSocket::Handler {
public:
	enum Error {
		no_error = 0 ,
		connection_error ,
		connection_timeout
	};

	enum CmdStatus {
		cmd_done,
		cmd_cont
	};

	typedef int CmdId;
	typedef std::function<CmdStatus( Error , const std::string& , std::string & )> CmdCallback;

private:
	std::unordered_map<CmdId, CmdCallback> commands;

	CmdId next_cmd_id;

	std::shared_ptr<TcpClientSocket> socket;	
public:
	IksConnection( boost::asio::io_service& io
				 , const std::string& server
				 , const std::string& port );

	CmdId send_command( const std::string& cmd
					  , const std::string& data
					  , CmdCallback callback );

	void send_command( CmdId id
					 , const std::string& cmd
					 , const std::string& data );

	void remove_command(CmdId id);

	void handle_read_line( boost::asio::streambuf& buf );

	void handle_error( const boost::system::error_code& ec );

	void handle_connected();

	void handle_disconnected();

	void connect();

	void disconnect();

	~IksConnection();

	boost::signals2::signal<void( )>							connected_sig;
	boost::signals2::signal<void( boost::system::error_code )>	connection_error_sig;
	boost::signals2::signal<void( const std::string&
							    , CmdId
								, const std::string& )>			cmd_sig;
};

#endif
/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
