#ifndef __TCPCONN_H_
#define __TCPCONN_H_

#include "baseconn.h"

/** Exception specific to TcpConnection class. */
class TcpConnectionException : public ConnectionException { } ;

/** 
 * Base class for TCP clients.
 */
class TcpConnection : public BaseConnection {
public:
	TcpConnection(struct event_base* base);
	virtual ~TcpConnection();

	/** Initializes client with given address:port.
	 * @param address IP address to connect to, in dot format
	 * @param port port number to connect to
	 */
	void InitTcp(std::string address, int port);

	/* BaseConnection interface */
	virtual void Open();
	virtual void Close();
	virtual void WriteData(const void* data, size_t size);
	virtual bool Ready() const;
	virtual void SetConfiguration(const SerialPortConfiguration& serial_conf);

	static void ReadDataCallback(struct bufferevent *bufev, void* ds);
	static void ErrorCallback(struct bufferevent *bufev, short event, void* ds);
protected:
	/** Creates a new nonblocking socket, using address and port stored previously. */
	void CreateSocket();
	/** Actual reading function, called by ReadCallback(). */
	void ReadData(struct bufferevent *bufev);
	/** Actual error function, called by ErrorCallback(). */
	void Error(struct bufferevent *bufev, short event);

	virtual void OpenFinished();
protected:
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
