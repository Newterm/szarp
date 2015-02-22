#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#include "bserialport.h"

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
	virtual void SetConfiguration(const SerialPortConfiguration& serial_conf);
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

#endif // __SERIALPORT_H__
