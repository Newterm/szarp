#ifndef __BSERIALPORT_H_
#define __BSERIALPORT_H_

#include "baseconn.h"
#include <termios.h>
#include <event.h>

class SerialPortException : public ConnectionException {
	SZ_INHERIT_CONSTR(SerialPortException, ConnectionException)
};

/** Provides interface to a bufferevent-driven serial port.
 */
class BaseSerialPort : public BaseConnection
{
protected:
	struct event_base *m_event_base;
public:
	BaseSerialPort(struct event_base* base): BaseConnection(), m_event_base(base) {}
	/** Set DTR and RTS signals according to params */
	virtual void LineControl(bool dtr, bool rts) = 0;

	/** Store SerialPortConfiguration to provided termios */
	static void SerialPortConfigurationToTermios(
		const SerialPortConfiguration& config, struct termios* termios);
};

#endif // __BSERIALPORT_H__
