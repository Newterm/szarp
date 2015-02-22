#ifndef __BSERIALPORT_H_
#define __BSERIALPORT_H_

#include "baseconn.h"
#include <termios.h>

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

	/** Store SerialPortConfiguration to provided termios */
	static void SerialPortConfigurationToTermios(
		const SerialPortConfiguration& config, struct termios* termios);
};

#endif // __BSERIALPORT_H__
