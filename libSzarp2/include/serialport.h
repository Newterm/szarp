#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

/** @file Classes providing serial port handling via:
 * 1) file descriptor (SerialPort)
 * 2) SerialAdapter, connected via TCP IP with data and command port
 */

// start of SerialAdapter-specific definitions
#ifndef B921600
	#define	B921600	(B460800 + 1)
#endif

// SerialAdapter commands
#define	CMD_16		16
#define	CMD_17		17
#define	CMD_LINECTRL	18
#define	CMD_19		19
#define	CMD_20		20
#define	CMD_21		21
#define	CMD_22		22
#define	CMD_23		23
#define	CMD_24		24
#define	CMD_33		33
#define	CMD_34		34
#define	CMD_36		36
#define	CMD_37		37
#define	CMD_38		38
#define	CMD_POLLALIVE	39
#define	CMD_ALIVE	40
#define	CMD_43		43
#define	CMD_PORT_INIT	44
#define	CMD_47		47
#define	CMD_SEND_FIFO	48
#define CMD_51		51
#define CMD_52		52

// serial port configuration
#define	SERIAL_B300		0
#define	SERIAL_B600		1
#define	SERIAL_B1200		2
#define	SERIAL_B2400		3
#define	SERIAL_B4800		4
#define	SERIAL_B7200		5
#define	SERIAL_B9600		6
#define	SERIAL_B19200		7
#define	SERIAL_B38400		8
#define	SERIAL_B57600		9
#define	SERIAL_B115200		10
#define	SERIAL_B230400		11
#define	SERIAL_B460800		12
#define	SERIAL_B921600		13
#define	SERIAL_B150		14
#define	SERIAL_B134		15
#define	SERIAL_B110		16
#define	SERIAL_B75		17
#define	SERIAL_B50		18

#define	SERIAL_BITS8		3
#define	SERIAL_BITS7		2
#define	SERIAL_BITS6		1
#define	SERIAL_BITS5		0

#define	SERIAL_STOP1		0
#define	SERIAL_STOP2		4

#define	SERIAL_EVEN		8
#define	SERIAL_ODD		16
#define	SERIAL_MARK		24
#define	SERIAL_SPACE		32
#define	SERIAL_NONE		0
// end of SerialAdapter-specific definitions


#include <event.h>
#include <map>
#include <netinet/in.h>
#include <string>
#include <fcntl.h>
#include <termios.h>
#include <vector>

class BaseConnection;

/** Listener receiving notifications from Connection */
class ConnectionListener
{
public:
	/** Callback for informing listener that the connection is open */
	virtual void OpenFinished(const BaseConnection *conn) = 0;
	/** Callback for notifications from SerialPort */
	virtual void ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data) = 0;
	/** Callback for error notifications from SerialPort.
	 * After an error is received, the listener responsible for port
	 * should try to close and open it once again */
	virtual void ReadError(const BaseConnection *conn, short int event) = 0;
	/** Callback for informing listener that his call to
	 * SetConfiguration() has been processed and connection is ready
	 * for writing
	 */
	virtual void SetConfigurationFinished(const BaseConnection *conn) = 0;
};

#include "exception.h"

class ConnectionException : public MsgException {};

/** Provides interface to a bufferevent-driven generalized connection.
 * User of this class is responsible for handling exceptions, errors
 * and restarting connection if neccessary.
 * All errors are reported to ConnectionListener via ReadError callback
 */
class BaseConnection
{
public:
	BaseConnection(struct event_base *base)
		:m_event_base(base)
	{}
	virtual ~BaseConnection() {}

	/** Open connection (previously initialized) */
	virtual void Open() = 0;
	/** Close connection */
	virtual void Close() = 0;
	/** Write data to connection */
	virtual void WriteData(const void* data, size_t size) = 0;
	/** Returns true if connection is ready for communication */
	virtual bool Ready() const = 0;
	/** Set line configuration for an already open port */
	virtual void SetConfiguration(const struct termios *serial_conf) = 0;

	/** Adds listener for port */
	void AddListener(ConnectionListener* listener)
	{
		m_listeners.push_back(listener);
	}
	/** Removes all listeners */
	void ClearListeners()
	{
		m_listeners.clear();
	}

protected:
	/** Callbacks, notify listeners */
	void OpenFinished();
	void ReadData(const std::vector<unsigned char>& data);
	void SetConfigurationFinished();
	void Error(short int event);

protected:
	struct event_base* const m_event_base;

private:
	/** Avoid copying */
	BaseConnection(const BaseConnection&);
	BaseConnection& operator=(const BaseConnection&);

	std::vector<ConnectionListener*> m_listeners;
};

#include "utils.h"
#include <map>
#include <netinet/in.h>
#include <string>

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
	virtual void SetConfiguration(const struct termios *serial_conf)
	{}

	static void ReadDataCallback(struct bufferevent *bufev, void* ds);
	static void ErrorCallback(struct bufferevent *bufev, short event, void* ds);
protected:
	/** Creates a new nonblocking socket, using address and port stored previously. */
	void CreateSocket();
	/** Actual reading function, called by ReadCallback(). */
	void ReadData(struct bufferevent *bufev);
	/** Actual error function, called by ErrorCallback(). */
	void Error(struct bufferevent *bufev, short event);

	void OpenFinished();
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


class SerialPortException : public ConnectionException {};

/** Provides interface to a bufferevent-driven serial port.
 */
class BaseSerialPort : public BaseConnection
{
public:
	BaseSerialPort(struct event_base* base)
		:BaseConnection(base)
	{}
	/** Set DTR and RTS signals according to params */
	virtual void LineControl(bool dtr, bool rts) = 0;
};


/** Serial port accessible via file descriptor */
class SerialPort: public BaseSerialPort
{
public:
	SerialPort(struct event_base* base)
		:BaseSerialPort(base), m_fd(-1), m_bufferevent(NULL)
	{}
	~SerialPort()
	{
		Close();
	}
	/** Open port (previously configured) */
	virtual void Open();
	/** true if ready for r/w */
	virtual bool Ready() const;
	/** Close port */
	virtual void Close();
	/** Set serial line configuration */
	virtual void SetConfiguration(const struct termios *serial_conf);
	/** Set DTR and RTS signals according to params */
	virtual void LineControl(bool dtr, bool rts);
	/** Write data to serial port */
	virtual void WriteData(const void* data, size_t size);

	void Init(const std::string& device_path)
	{
		m_device_path = device_path;
	}

	static void ReadDataCallback(struct bufferevent *bufev, void* ds);
	static void ErrorCallback(struct bufferevent *bufev, short event, void* ds);
protected:
	/** Actual reading function, called by ReadCallback(). */
	void ReadData(struct bufferevent *bufev);
	/** Actual error function, called by ErrorCallback(). */
	void Error(struct bufferevent *bufev, short event);

	std::string m_device_path;
	int m_fd;
	struct bufferevent* m_bufferevent;	/**< Libevent bufferevent for port access. */
};


/** Exception specific to SerialAdapter class. */
class SerialAdapterException : public SerialPortException { } ;

/** 
 * Base class for SerialAdapter clients.
 */
class SerialAdapter: public BaseSerialPort, public ConnectionListener {
public:
	static const int DEFAULT_DATA_PORT = 950;
	static const int DEFAULT_CMD_PORT = 966;

	SerialAdapter(struct event_base*,
		bool enable_fifo=true, bool server_CN2500 = false);
	virtual ~SerialAdapter();

	/** BaseSerialPort interface */
	virtual void Open();
	virtual bool Ready() const;
	virtual void Close();
	virtual void SetConfiguration(const struct termios *serial_conf);
	virtual void LineControl(bool dtr, bool rts);
	virtual void WriteData(const void* data, size_t size);

	/** ConnectionListener interface */
	virtual void OpenFinished(const BaseConnection *conn);
	virtual void ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data);
	virtual void ReadError(const BaseConnection *conn, short int event);
	virtual void SetConfigurationFinished(const BaseConnection *conn)
	{}

	/** Initializes client with given address:port.
	 * @param address IP address to connect to, in dot format
	 * @param port port number to connect to
	 */
	void InitTcp(std::string address, int data_port=DEFAULT_DATA_PORT,
		int cmd_port=DEFAULT_CMD_PORT);

private:
	void ProcessCmdResponse(const std::vector<unsigned char> &cmd_buf);
	void WriteCmd(std::vector<unsigned char> &cmd_buffer);

private:
	typedef enum {UNINITIALIZED, SETCONF1, SETCONF2, SETCONF3, READY} SerialConfState;

	const bool m_enable_fifo;
	const bool m_server_CN2500;

	TcpConnection* m_data_connection;	/**< connection used for data send/recv with serial port */
	TcpConnection* m_cmd_connection;		/**< connection used for communication with SerialAdapter driver */

	SerialConfState m_serial_state;		/**< state of set serial configuration communication */
	bool m_open_pending;
	std::vector<unsigned char> m_last_serial_conf_cmd;
};

#endif // __SERIALPORT_H__
