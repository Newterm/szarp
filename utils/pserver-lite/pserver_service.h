#ifndef  __PSERVER_SERVICE_H
#define  __PSERVER_SERVICE_H

#include <string>
#include "tcpserver.h"

class PServerService : public TcpServerListener
{
public:
	PServerService (PEventBase event_base, const std::string& addr, const unsigned short port) { };
	~PServerService (void) { };

	void AcceptConnection (TcpServer *server, PTcpServerConnection conn) override { };
	void AcceptError (TcpServer *server, int error_code, std::string error_str) override { };
	void ReadData (TcpServerConnection *conn, const std::vector<unsigned char>& data) override { };
	void ReadError (TcpServerConnection *conn, int error_code, std::string error_str) override { };

	void run(void);
	void process_request(std::string& msg_received);
};

#endif  /* __PSERVER_SERVICE_H */
