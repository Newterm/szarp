#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "exception.h"
#include "evbase.h"
#include <string>
#include <vector>

/** TCP server using observer pattern
 */

class TcpServer;
class TcpServerConnection;

typedef std::shared_ptr<TcpServer> PTcpServer;
typedef std::shared_ptr<TcpServerConnection> PTcpServerConnection;


/**
 * Listener receiving notifications from TcpServer and TcpServerConnections.
 * To create a useful TCP Server, you should:
 * - create a class deriving from TcpServerListener
 * - listen to an TCPServer instance
 */
class TcpServerListener
{
public:
	virtual ~TcpServerListener() {}

	/** Receives new client connection */
	virtual void AcceptConnection(TcpServer *server, PTcpServerConnection conn) = 0;

	/** Receives standard errno codes + string version */
	virtual void AcceptError(TcpServer *server, int error_code, std::string error_str) = 0;

	/** Data read from client connection */
	virtual void ReadData(TcpServerConnection *conn, const std::vector<unsigned char>& data) = 0;

	/** Receives standard errno codes + string version and: -1, EOF when connection closes */
	virtual void ReadError(TcpServerConnection *conn, int error_code, std::string error_str) = 0;
};


/**
 * Implements observer pattern
 */
class TcpServerObservable
{
public:
	void AddListener(TcpServerListener* listener)
	{
		m_listeners.push_back(listener);
	}

	void ClearListeners()
	{
		m_listeners.clear();
	}
protected:
	std::vector<TcpServerListener*> m_listeners;
};


/** Exception specific to TcpServer and TcpServerConnection */
class TcpServerException : public SzException {
	SZ_INHERIT_CONSTR(TcpServerException, SzException)
};


/**
 * TCP Server using observer pattern.
 */
class TcpServer final: public TcpServerObservable
{
public:
	TcpServer(PEventBase base);

	/** To listen on all addressess, use "*" as address */
	void InitTcp(const std::string address, const int port);

	/** Called when new connection was be accepted. */
	static void AcceptCallback(struct evconnlistener *listener,
		evutil_socket_t fd, struct sockaddr *address, int socklen,
		void *ds);

	/** Called when an error occured while accepting a new connection. */
	static void AcceptErrorCallback(struct evconnlistener *listener,
		void *ds);

protected:
	/** Actual accept method, called by static callback */
	void AcceptConnection(evutil_socket_t fd);
	void AcceptError();

protected:
	std::string m_address;	/**< IP address to listen on */
	int m_listen_port;	/**< TCP port to listen on. */
	PEvconnListener m_listener;	/**< event for listening for connections. */
	PEventBase m_event_base;	/**< event base for creating events */
};


/**
 * Accepted client connection to the TCP server
 * After creation, reads and writes are disabled. To communicate
 * with the connection you should:
 * - first add your class as a listener to the connection
 * - then call Enable() on the connection
 */
class TcpServerConnection final: public TcpServerObservable
{
public:
	/** Should be called by TcpServer only */
	TcpServerConnection(PBufferevent bufev);

	/** Enable read/write (disabled by default) */
	void Enable();

	/** If connection is lost, will throw TcpServerException */
	void WriteData(const void* data, size_t size);

	/** After subsequent write, the underlying connection will be closed.
	 * If no write is performed, nothing will happen.
	 * All data written in subsequent WriteData call is guaranteed
	 * to be flushed.
	 */
	void CloseOnWriteFinished();

	/** libevent callbacks */
	static void ReadDataCallback(struct bufferevent *bufev, void* ds);
	static void WriteDataCallback(struct bufferevent *bufev, void* ds);
	static void ErrorCallback(struct bufferevent *bufev, short event, void* ds);
protected:
	/* Methods called by static callbacks */
	void ReadData(struct bufferevent *bufev);
	void WriteFinished(struct bufferevent *bufev);
	void Error(struct bufferevent *bufev, short event);

protected:
	PBufferevent m_bufferevent;
	bool m_close_on_write_finished;
};

#endif // __TCPSERVER_H__
