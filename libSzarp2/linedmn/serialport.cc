#include "serialport.h"
#include "daemonutils.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

void SerialPort::Open()
{
	if (m_bufferevent != NULL) {
		throw ConnectionException("Open already pending");
	}
	m_fd = open(m_device_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
	if (m_fd == -1) {
		throw SerialPortException(errno, "Couldn't open device " + m_device_path);
	}
	m_bufferevent = bufferevent_socket_new(m_event_base, m_fd, BEV_OPT_CLOSE_ON_FREE);
	if (m_bufferevent == NULL) {
		Close();
		throw SerialPortException("Error creating bufferevent.");
	}
	bufferevent_setcb(m_bufferevent, ReadDataCallback, NULL, ErrorCallback, this);
	bufferevent_enable(m_bufferevent, EV_READ | EV_WRITE | EV_PERSIST);
	OpenFinished();
}

bool SerialPort::Ready() const
{
	return (m_bufferevent != NULL);
}

void SerialPort::Close()
{
	if (m_bufferevent != NULL) {
		bufferevent_free(m_bufferevent);
		m_bufferevent = NULL;
	}
	if (m_fd > 0) {
		close(m_fd);
		m_fd = -1;
	}
}

void SerialPort::SetConfiguration(const SerialPortConfiguration& config)
{
	struct termios termios;
	if (tcgetattr(m_fd, &termios) != 0) {
		Close();
		throw SerialPortException(errno, "Cannot get port settings");
	}

	SerialPortConfigurationToTermios(config, &termios);

	if (tcsetattr(m_fd, TCSANOW, &termios) != 0) {
		Close();
		throw SerialPortException(errno, "Cannot set port settings");
	}
	SetConfigurationFinished();
}

void SerialPort::LineControl(bool dtr, bool rts)
{
	int control;
	ioctl(m_fd, TIOCMGET, &control);
	if (dtr)
		control |= TIOCM_DTR;
	else
		control &= ~TIOCM_DTR;
	if (rts)
		control |= TIOCM_RTS;
	else
		control &= ~TIOCM_RTS;
	ioctl(m_fd, TIOCMSET, &control);
}

void SerialPort::WriteData(const void* data, size_t size)
{
	if (m_bufferevent == NULL) {
		throw SerialPortException("Port is currently unavailable");
	}
	int ret = bufferevent_write(m_bufferevent, data, size);
	if (ret < 0) {
		throw SerialPortException("Write data failed.");
	}
}

void SerialPort::ReadDataCallback(struct bufferevent *bufev, void* ds)
{
	reinterpret_cast<SerialPort*>(ds)->ReadData(bufev);
}

void SerialPort::ErrorCallback(struct bufferevent *bufev, short event, void* ds)
{
	reinterpret_cast<SerialPort*>(ds)->Error(bufev, event);
}

void SerialPort::ReadData(struct bufferevent *bufev)
{
	std::vector<unsigned char> data;
	unsigned char c;
	while (bufferevent_read(m_bufferevent, &c, 1) == 1) {
		data.push_back(c);
	}
	BaseConnection::ReadData(data);
}

void SerialPort::Error(struct bufferevent *bufev, short event)
{
	// Notify listeners
	BaseConnection::Error(event);
}

template <>
SerialPort* BaseConnFactory::create_from_unit(struct event_base *base, UnitInfo *unit) {
	auto conn = new SerialPort(base);
	auto device_path = unit->getAttribute("extra:path");
	conn->Init(device_path);
	return conn;
}
