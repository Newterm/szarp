#ifndef __BASECONN_H_
#define __BASECONN_H_

#include "exception.h"
#include <event.h>
#include <string>
#include <vector>

/** @file Virtual connection using observer pattern
 */

/**self-descriptive struct holding all aspects of serial port conifguration in one place*/
class SerialPortConfiguration {
public:
	SerialPortConfiguration()
	:
	path(""), parity(NONE), stop_bits(1), speed(0), char_size(CS_8)
	{}

	std::string path;
	enum PARITY {
		NONE,
		ODD,
		EVEN,
		MARK,
		SPACE
	} parity;
	int stop_bits;
	int speed;
	enum CHAR_SIZE {
		CS_5,
		CS_6,
		CS_7,
		CS_8
	} char_size;
};

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
	virtual void SetConfiguration(const SerialPortConfiguration& serial_conf) = 0;

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

#endif // __BASECONN_H__
