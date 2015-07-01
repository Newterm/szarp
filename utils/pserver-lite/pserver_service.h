#ifndef  __PSERVER_SERVICE_H
#define  __PSERVER_SERVICE_H

#include <string>
#include <map>
#include "tcpserver.h"

class PServerService : public TcpServerListener
{
public:
	PServerService (std::string addr, unsigned short port);
	~PServerService (void) { };

	void AcceptConnection (TcpServer *server, PTcpServerConnection conn) override;
	void AcceptError (TcpServer *server, int error_code, std::string error_str) override;
	void ReadData (TcpServerConnection *conn, const std::vector<unsigned char>& data) override;
	void ReadError (TcpServerConnection *conn, int error_code, std::string error_str) override;

	void run (void);
	void process_request (TcpServerConnection *conn, std::string msg_received);

private:
	PEventBase m_event_base;
	PTcpServer m_tcp_server;
	std::map<TcpServerConnection*, PTcpServerConnection> m_connections;
};

#endif  /* __PSERVER_SERVICE_H */
