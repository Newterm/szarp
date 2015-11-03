#ifndef IKS_CLIENT_H
#define IKS_CLIENT_H
#include <unordered_map>

#include "tcp_client_socket.h"

class IksClient : public std::enable_shared_from_this<IksClient>, public TcpClientSocket::Handler {
public:
	enum class Error : int {
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
	IksClient( boost::asio::io_service& io , const std::string& server, const std::string& port );

	CmdId send_command(
		const std::string& cmd,
		const std::string& data,
		CmdCallback callback );

	void send_command(
		CmdId id,
		const std::string& cmd,
		const std::string& data
		);

	void remove_command(CmdId id);

	void handle_read_line( boost::asio::streambuf& buf );

	void handle_error( const boost::system::error_code& ec );

	~IksClient();
};

#endif
