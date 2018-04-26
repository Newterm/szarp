#ifndef __TCPSERV_CONN_H_
#define __TCPSERV_CONN_H_

#include "baseconn.h"
#include "szarp_config.h"
#include "custom_assert.h"

class BaseServerConnectionHandler: public BaseConnection {
public:
	using BaseConnection::BaseConnection;
	virtual void CloseConnection(const BaseConnection*) = 0;
	void WriteData(const void* data, size_t size) override;
	void SetConfiguration(const SerialPortConfiguration& serial_conf) override;
};

class FdTcpServerConnection: public BaseConnection {
	BaseServerConnectionHandler* handler;
	struct bufferevent *m_bufferevent;
	int m_fd;

	// private - only handler can create
	friend class TcpServerConnectionHandler;
	FdTcpServerConnection(BaseServerConnectionHandler* _handler, struct event_base *ev_base);

public:
	~FdTcpServerConnection();

	void InitConnection(int fd);
	void Init(UnitInfo* unit) override;

	void Open() override;
	void Close() override;

	/** Returns true if connection is ready for communication */
	bool Ready() const override;
	void ReadData(struct bufferevent *bufev);
	void WriteData(const void* data, size_t size) override;

	/** Set line configuration for an already open port */
	void SetConfiguration(const SerialPortConfiguration& serial_conf) override;

	static void ErrorCallback(struct bufferevent *bufev, short event, void* conn);
	static void ReadDataCallback(struct bufferevent *bufev, void* conn);
};

// implements handling incoming connections
class TcpServerConnectionHandler: public BaseServerConnectionHandler, public ConnectionListener {
	int m_fd = -1;

	struct event _event;
	struct sockaddr_in m_addr;

	std::set<unsigned long> m_allowed_ips;
	std::list<std::unique_ptr<BaseConnection>> m_connections;

public:
	using BaseServerConnectionHandler::BaseServerConnectionHandler;
	void Init(UnitInfo* unit);

private:
	void ConfigureAllowedIps(UnitInfo* unit);

	// socket handling
	int start_listening();
	bool ip_is_allowed(struct sockaddr_in in_s_addr) const;
	boost::optional<int> accept_socket(int in_fd);

	static void connection_accepted_cb(int _fd, short event, void* _handler);
	void AcceptConnection(int _fd);

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

#endif
