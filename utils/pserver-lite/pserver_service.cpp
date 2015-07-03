
#include <iostream>
#include "pserver_service.h"
#include "command_factory.h"

PServerService::PServerService (std::string addr, unsigned short port)
{
	/* create event base */
	PEvBase evbase = std::shared_ptr<struct event_base>(event_base_new(), event_base_free);
	m_event_base = std::make_shared<EventBase>(evbase);

	m_tcp_server = std::make_shared<TcpServer>(m_event_base);
	m_tcp_server->AddListener(this);
	m_tcp_server->InitTcp(addr, port);
}

void PServerService::AcceptConnection (TcpServer *server,
		PTcpServerConnection conn)
{
	conn->AddListener(this);
	conn->Enable();
	m_connections[conn.get()] = conn;
}

void PServerService::AcceptError (TcpServer *server,
		int error_code, std::string error_str)
{
	/* empty */;
}

void PServerService::ReadData (TcpServerConnection *conn,
		const std::vector<unsigned char>& data)
{
	std::string msg(data.begin(), data.end());
	process_request(conn, msg);
}

void PServerService::ReadError (TcpServerConnection *conn,
		int error_code, std::string error_str)
{
	m_connections.erase(conn);
}

void PServerService::run (void)
{
	/* start main server loop */
	m_event_base->Dispatch();
}

void PServerService::process_request (TcpServerConnection *conn,
		std::string msg_received)
{
	try {
		auto tokens = CommandHandler::tokenize(msg_received);

		/* fetch command tag and remove it from the vector of tokens */
#if GCC_VERSION >= 40900
		auto iter = tokens.cbegin();
#else
		auto iter = tokens.begin();
#endif
		std::string cmd_tag = *iter;
		tokens.erase(iter);

		/* execute command */
		auto cmd_handler = CommandFactory::make_cmd(cmd_tag);

		cmd_handler->load_args(tokens);
		auto reply_msg = cmd_handler->exec();

		/* send reply message */
		conn->WriteData(reply_msg.data(), reply_msg.size());
	}
	catch (CommandHandler::Exception& ex) {
		std::string err("ERROR 400 ");
		err += ex.what();
		err += std::string("\n");

		conn->WriteData(err.data(), err.size());
	}
	catch (std::runtime_error& ex) {
		std::string err("ERROR 400 ");
		err += ex.what();
		err += std::string("\n");

		conn->WriteData(err.data(), err.size());
	}
}
