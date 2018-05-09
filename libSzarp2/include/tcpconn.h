#ifndef __TCPCONN_H_
#define __TCPCONN_H_

#include "baseconn.h"
#include <event.h>
class UnitInfo;

/** Exception specific to TcpConnection class. */
class TcpConnectionException : public ConnectionException {
	SZ_INHERIT_CONSTR(TcpConnectionException, ConnectionException)
};

/**
 * Base class for TCP clients.
 */
class TcpConnection : public BaseConnection {
public:
	TcpConnection(struct event_base* base);
	~TcpConnection() override;

	/** Initializes client with given address:port.
	 * @param address IP address to connect to, in dot format
	 * @param port port number to connect to
	 */
	void InitTcp(const std::string& address, int port);

	void Init(UnitInfo* unit) override;

	/* BaseConnection interface */
	void Open() override;
	void Close() override;
	void WriteData(const void* data, size_t size) override;
	bool Ready() const override;
	void SetConfiguration(const SerialPortConfiguration& serial_conf) override;

	static void ReadDataCallback(struct bufferevent *bufev, void* ds);
	static void ErrorCallback(struct bufferevent *bufev, short event, void* ds);
protected:
	/** Creates a new nonblocking socket, using address and port stored previously. */
	void CreateSocket();
	/** Actual reading function, called by ReadCallback(). */
	void ReadData(struct bufferevent *bufev);
	/** Actual error function, called by ErrorCallback(). */
	void Error(struct bufferevent *bufev, short event);

	void OpenFinished() override;
protected:
	struct event_base *m_event_base;
	int m_connect_fd;	/**< Client connecting socket file descriptor. */
	bool m_connected;	/**< Client socket connected to server. */
	std::string m_address;	/**< address to connect to */
	int m_port;		/**< port to connect to*/
	std::string m_id;	/**< client id */
	struct sockaddr_in m_server_addr;	/**< Server full address. */
	struct bufferevent* m_bufferevent;	/**< Libevent bufferevent for socket read/write. */
	bool m_connection_open;	/**< true if connection is currently open */
};

#endif // __TCPCONN_H__
