#ifndef IKS_LOCATION_CONN_H
#define IKS_LOCATION_CONN_H

#include "iks_connection.h"

class IksLocationCmdReceiver : public IksCmdReceiver {
public:
	enum Error {
		failed_to_list_remotes, 
		location_not_present,
		invalid_response, 
		failed_to_connect
	};

	void on_connected_to_location();

	void on_location_connection_error();

};

class IksLocationConnection : public std::enable_shared_from_this<IksLocationConnection>, public IksCmdReceiver {
	std::string location;
	std::string type;

	IksConnection connection;
	IksCmdReceiver* receiver;

	void connect();
public:
	IksLocationConnection( boost::asio::io_service& io
						 , const std::string& location
						 , const std::string& type
						 , const std::string& server
						 , const std::string& port
						 , IksCmdReceiver *receiver);

	IksConnection::CmdId send_command(
		const std::string& cmd,
		const std::string& data,
		IksConnection::CmdCallback callback );

	void send_command(
		IksConnection::CmdId id,
		const std::string& cmd,
		const std::string& data
		);

	void on_cmd(const std::string& status, IksConnection::CmdId id, std::string& data);

};
#endif
