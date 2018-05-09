#ifndef __TCPSERV_CONN_H_
#define __TCPSERV_CONN_H_

#include "baseconn.h"
#include "szarp_config.h"
#include "custom_assert.h"

#include "evbase.h"

class BaseServerConnectionHandler: public BaseConnection {
public:
	using BaseConnection::BaseConnection;
	virtual void CloseConnection(const BaseConnection*) = 0;
	void WriteData(const void* data, size_t size) override;
	void SetConfiguration(const SerialPortConfiguration& serial_conf) override;
};

// implements handling incoming connections
class TcpServerConnectionHandler: public BaseServerConnectionHandler, public ConnectionListener {
private:
	PEventBase m_event_base;
	PEvconnListener m_listener;	/**< event for listening for connections. */

	struct sockaddr_in addr;

	std::set<unsigned long> m_allowed_ips;
	std::list<std::unique_ptr<BaseConnection>> m_connections;

public:
	TcpServerConnectionHandler(struct event_base* base);
	void Init(UnitInfo* unit);

private:
	void ConfigureAllowedIps(UnitInfo* unit);
	bool ip_is_allowed(struct sockaddr_in *in_s_addr) const;

	static void AcceptErrorCallback(struct evconnlistener *listener, void *ds);
	static void AcceptCallback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ds);

	void AcceptConnection(evutil_socket_t fd, struct sockaddr *addr);

public:
	// Connection Listener implementation
	void OpenFinished(const BaseConnection *conn) override;
	void ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data) override;
	void ReadError(const BaseConnection *conn, short int event) override;
	void SetConfigurationFinished(const BaseConnection *conn) override;
	void CloseConnection(const BaseConnection* conn) override;

	// BaseConnection implementation
	void Open() override;
	void Close() override;
	bool Ready() const override;
};

class TcpBaseServerConnection: public BaseConnection {
private:
	BaseServerConnectionHandler* handler;
	PBufferevent m_bufferevent;

	// private - only handler can create an instance of this class
	friend class TcpServerConnectionHandler;
	TcpBaseServerConnection(BaseServerConnectionHandler* _handler, struct event_base *ev_base);

public:
	TcpBaseServerConnection(BaseServerConnectionHandler* _handler, PBufferevent bufev);

	void Init(UnitInfo* unit) override;
	void SetConfiguration(const SerialPortConfiguration& serial_conf) override;

	bool Ready() const override;
	void Open() override;
	void Close() override;
	void WriteData(const void* data, size_t size) override;

	static void ReadDataCallback(struct bufferevent *bufev, void* ds);
	static void ErrorCallback(struct bufferevent *bufev, short event, void* ds);
};

#endif
