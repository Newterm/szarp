
#include "serialport.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <ostream>
#include <linux/serial_reg.h>
#include <linux/serial.h>
#include <iostream>
#include <errno.h>
#include <sys/ioctl.h>

void BaseConnection::OpenFinished()
{
	for (auto* listener : m_listeners) {
		listener->OpenFinished(this);
	}
}

void BaseConnection::ReadData(const std::vector<unsigned char>& data)
{
	for (auto* listener : m_listeners) {
		listener->ReadData(this, data);
	}
}

void BaseConnection::Error(short int event)
{
	for (auto* listener : m_listeners) {
		listener->ReadError(this, event);
	}
}

void BaseConnection::SetConfigurationFinished()
{
	for (auto* listener : m_listeners) {
		listener->SetConfigurationFinished(this);
	}
}

void SerialPort::Open()
{
	if (m_bufferevent != NULL) {
		return;
	}
	m_fd = open(m_device_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
	if (m_fd == -1) {
		SerialPortException ex;
		ex.SetMsg("Couldn't open device '%s', errno: %d (%s)",
			m_device_path.c_str(), errno, strerror(errno));
		throw ex;
	}
	m_bufferevent = bufferevent_socket_new(m_event_base, m_fd, BEV_OPT_CLOSE_ON_FREE);
	if (m_bufferevent == NULL) {
		Close();
		SerialPortException ex;
		ex.SetMsg("Error creating bufferevent.");
		throw ex;
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

void SerialPort::SetConfiguration(const struct termios *serial_conf)
{
	if (tcsetattr(m_fd, TCSANOW, serial_conf) == -1) {
		Close();
		SerialPortException ex;
		ex.SetMsg("Cannot set port settings, errno: %d (%s)",
			errno, strerror(errno));
		throw ex;
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
	if (m_bufferevent == NULL)
	{
		SerialPortException ex;
		ex.SetMsg("Port is currently unavailable");
		throw ex;
	}
	int ret = bufferevent_write(m_bufferevent, data, size);
	if (ret < 0)
	{
		SerialPortException ex;
		ex.SetMsg("Write data failed.");
		throw ex;
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


SerialAdapter::SerialAdapter(struct event_base* base,
		bool enable_fifo, bool server_CN2500)
	:
	BaseSerialPort(base),
	m_enable_fifo(enable_fifo), m_server_CN2500(server_CN2500),
	m_serial_state(UNINITIALIZED), m_open_pending(false)
{
	m_data_connection = new TcpConnection(base);
	m_cmd_connection = new TcpConnection(base);
	m_data_connection->AddListener(this);
	m_cmd_connection->AddListener(this);
}

SerialAdapter::~SerialAdapter()
{
	Close();
	delete m_data_connection;
	delete m_cmd_connection;
}

void SerialAdapter::InitTcp(std::string address, int data_port, int cmd_port)
{
	m_data_connection->InitTcp(address, data_port);
	m_cmd_connection->InitTcp(address, cmd_port);
}

void SerialAdapter::Open()
{
	if (m_open_pending) {
		return;
	}
	try {
		m_data_connection->Open();
		m_cmd_connection->Open();
		m_open_pending = true;
	} catch (const TcpConnectionException& e) {
		Close();
		SerialAdapterException ex;
		ex.SetMsg(e.what());
		throw ex;
	}
}

void SerialAdapter::OpenFinished(const BaseConnection *conn)
{
	if (m_data_connection->Ready() && m_cmd_connection->Ready()) {
		m_open_pending = false;
		BaseConnection::OpenFinished();
	}
}

void SerialAdapter::Close()
{
	m_data_connection->Close();
	m_cmd_connection->Close();
	m_open_pending = false;
}

bool SerialAdapter::Ready() const
{
	return (m_data_connection->Ready()
		&& m_cmd_connection->Ready()
		&& (m_serial_state == READY));
}

void SerialAdapter::WriteData(const void* data, size_t size)
{
	if (not Ready()) {
		SerialAdapterException ex;
		ex.SetMsg("Port is currently unavailable");
		throw ex;
	}
	try {
		m_data_connection->WriteData(data, size);
	} catch (const TcpConnectionException& e) {
		SerialAdapterException ex;
		ex.SetMsg(e.what());
		throw ex;
	}
}

void SerialAdapter::WriteCmd(std::vector<unsigned char> &cmd_buffer)
{
	try {
		m_cmd_connection->WriteData(&cmd_buffer[0], cmd_buffer.size());
	} catch (const TcpConnectionException& e) {
		// Notify listeners that serial port is malfunctioning
		BaseConnection::Error(EVBUFFER_ERROR);
	}
	// we cannot flush socket bufferevents, so commands can be sent in a single packet
}
	
void SerialAdapter::ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data)
{
	if (conn == m_data_connection) {
		if (Ready()) {
			BaseConnection::ReadData(data);
		}
		// else data is probably garbage
	} else {
		ProcessCmdResponse(data);
	}
	// TODO else?
}

void SerialAdapter::ProcessCmdResponse(const std::vector<unsigned char> &cmd_buf)
{
	int bytes_left = cmd_buf.size();
	int command_size = 0;
	int index = 0;

	while (bytes_left > 0)
	{
		// process serial port configuration setting
		if ((m_serial_state == SETCONF1) && (cmd_buf[index] == CMD_PORT_INIT)) {
			int fifo_size;
			if (m_enable_fifo)
			{
				if (m_server_CN2500)
					fifo_size = 64;
				else
					fifo_size = 16;
			}
			else
				fifo_size = 1;

			std::vector<unsigned char> fifo_cmd_buffer;
			fifo_cmd_buffer.resize(3, 0);
			fifo_cmd_buffer[0] = CMD_SEND_FIFO;
			fifo_cmd_buffer[1] = 1;
			fifo_cmd_buffer[2] = fifo_size;
			m_serial_state = SETCONF2;
			WriteCmd(fifo_cmd_buffer);
		} else if ((m_serial_state == SETCONF2) && (cmd_buf[index] == CMD_SEND_FIFO)) {
			m_serial_state = SETCONF3;
			WriteCmd(m_last_serial_conf_cmd);
		} else if ((m_serial_state == SETCONF3) && (cmd_buf[index] == CMD_PORT_INIT)) {
			m_serial_state = READY;
			BaseConnection::SetConfigurationFinished();
		}

		if (cmd_buf[index] == CMD_POLLALIVE)
		{
			if (bytes_left < 3)
			{
				bytes_left = 0;
				continue;
			}
			std::vector<unsigned char> keepalive_resp;
			keepalive_resp.resize(3, 0);
			keepalive_resp[0] = CMD_ALIVE;
			keepalive_resp[1] = 1;
			keepalive_resp[2] = cmd_buf[index + 2];
			command_size = 3;
			WriteCmd(keepalive_resp);
			index += command_size;
			bytes_left -= command_size;
			continue;
		}
		switch (cmd_buf[index])
		{
		case CMD_38 :
		case CMD_47 :
		case CMD_22 :
		case CMD_21 :
			command_size = 4;
			break;
		case CMD_19 :
		case CMD_PORT_INIT :
			command_size = 5;
			break;
		case CMD_17:
		case CMD_16:
		case CMD_23:
		case CMD_LINECTRL:
		case CMD_33:
		case CMD_34:
		case CMD_36:
		case CMD_37:
		case CMD_20:
		case CMD_43:
		case CMD_SEND_FIFO:
		case CMD_24:
		case CMD_51:
		case CMD_52:
			command_size = 3;
			break;
		default :
			command_size = bytes_left;
			break;
		}

		index += command_size;
		bytes_left -= command_size;
	}
}

void SerialAdapter::ReadError(const BaseConnection *conn, short int event)
{
	if (not conn->Ready()) {
		Close();
	}
	// Notify listeners
	BaseSerialPort::Error(event);
}

void SerialAdapter::SetConfiguration(const struct termios *serial_conf)
{
	int32_t mode = serial_conf->c_cflag & CSIZE;
	if (mode == CS5)
		mode = SERIAL_BITS5;
	else if (mode == CS6)
		mode = SERIAL_BITS6;
	else if (mode == CS7)
		mode = SERIAL_BITS7;
	else if (mode == CS8)
		mode = SERIAL_BITS8;

	if (serial_conf->c_cflag & CSTOPB)
		mode |= SERIAL_STOP2;
	else
		mode |= SERIAL_STOP1;

	if (serial_conf->c_cflag & PARENB)
	{
		if (serial_conf->c_cflag & CMSPAR)
			if (serial_conf->c_cflag & PARODD)
				mode |= SERIAL_MARK;
			else
				mode |= SERIAL_SPACE;
		else
			if (serial_conf->c_cflag & PARODD)
				mode |= SERIAL_ODD;
			else
				mode |= SERIAL_EVEN;
	}
	else
		mode |= SERIAL_NONE;

	int baudrate_index;
	switch ( serial_conf->c_cflag & (CBAUD|CBAUDEX))
	{
	case B921600:
		baudrate_index = SERIAL_B921600;
		break;
	case B460800:
		baudrate_index = SERIAL_B460800;
		break;
	case B230400:
		baudrate_index = SERIAL_B230400;
		break;
	case B115200:
		baudrate_index = SERIAL_B115200;
		break;
	case B57600:
		baudrate_index = SERIAL_B57600;
		break;
	case B38400:
		baudrate_index = SERIAL_B38400;
		break;
	case B19200:
		baudrate_index = SERIAL_B19200;
		break;
	case B9600:
		baudrate_index = SERIAL_B9600;
		break;
	case B4800:
		baudrate_index = SERIAL_B4800;
		break;
	case B2400:
		baudrate_index = SERIAL_B2400;
		break;
	case B1800:
		baudrate_index = 0xff;
		break;
	case B1200:
		baudrate_index = SERIAL_B1200;
		break;
	case B600:
		baudrate_index = SERIAL_B600;
		break;
	case B300:
		baudrate_index = SERIAL_B300;
		break;
	case B200:
		baudrate_index = 0xff;
		break;
	case B150:
		baudrate_index = SERIAL_B150;
		break;
	case B134:
		baudrate_index = SERIAL_B134;
		break;
	case B110:
		baudrate_index = SERIAL_B110;
		break;
	case B75:
		baudrate_index = SERIAL_B75;
		break;
	case B50:
		baudrate_index = SERIAL_B50;
		break;
	default:
		baudrate_index = 0xff;
	}
	if (baudrate_index == 0xff)
	{
		SerialAdapterException ex;
		ex.SetMsg("Baudrate not supported");
		throw ex;
	}

	std::vector<unsigned char> cmd_buffer;
	cmd_buffer.resize(10, 0);

	cmd_buffer[0] = CMD_PORT_INIT;
	cmd_buffer[1] = 8;
	cmd_buffer[2] = baudrate_index;
	cmd_buffer[3] = mode;

	// line control
	int modem_control = 0;
	if (serial_conf->c_cflag & CBAUD)
		modem_control = UART_MCR_DTR | UART_MCR_RTS;
	if (modem_control & UART_MCR_DTR)
		cmd_buffer[4] = 1;
	else
		cmd_buffer[4] = 0;
	if (modem_control & UART_MCR_RTS)
		cmd_buffer[5] = 1;
	else
		cmd_buffer[5] = 0;

	// flow control
	if (serial_conf->c_cflag & CRTSCTS)
	{
		cmd_buffer[6] = 1;
		cmd_buffer[7] = 1;
	}
	else
	{
		cmd_buffer[6] = 0;
		cmd_buffer[7] = 0;
	}
	if (serial_conf->c_iflag & IXON)
	{
		cmd_buffer[8] = 1;
	}
	else
	{
		cmd_buffer[8] = 0;
	}
	if (serial_conf->c_iflag & IXOFF)
	{
		cmd_buffer[9] = 1;
	}
	else
	{
		cmd_buffer[9] = 0;
	}
	// cmd is ready to send
	m_serial_state = SETCONF1;
	
	m_last_serial_conf_cmd = cmd_buffer;
	if (not m_cmd_connection->Ready())
	{
		SerialAdapterException ex;
		ex.SetMsg("Port is currently unavailable for config");
		throw ex;
	}
	WriteCmd(cmd_buffer);
	// data shouldn't be read from data port until new serial settings negotiations are over

	// process response (set some termios flags differently) if necessary
}

void SerialAdapter::LineControl(bool dtr, bool rts)
{
	if (not Ready())
	{
		SerialAdapterException ex;
		ex.SetMsg("Port is currently unavailable");
		throw ex;
	}
	std::vector<unsigned char> cmd_buffer;
	cmd_buffer.resize(4, 0);

	cmd_buffer[0] = CMD_LINECTRL;
	cmd_buffer[1] = 2;
	if (dtr) {
		cmd_buffer[2] = 1;
	}
	if (rts) {
		cmd_buffer[3] = 1;
	}
	WriteCmd(cmd_buffer);
}

#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <iostream>


TcpConnection::TcpConnection(struct event_base* base)
	:BaseConnection(base),
	m_connect_fd(-1), m_bufferevent(NULL), m_connection_open(false)
{}

TcpConnection::~TcpConnection()
{
	if (m_connect_fd != -1) {
		Close();
	}
}

void TcpConnection::InitTcp(std::string address, int port)
{
	m_address = address;
	m_port = port;

	std::ostringstream s;
	s << m_address << ":" << m_port;
	m_id = s.str();

	/* server address */ 
	m_server_addr.sin_family = AF_INET;
	inet_aton(address.c_str(), &m_server_addr.sin_addr);
	m_server_addr.sin_port = htons(port);
}

void TcpConnection::Open()
{
	if (m_connect_fd != -1) {
		return;
	}
	CreateSocket();

	m_bufferevent = bufferevent_socket_new(m_event_base, m_connect_fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
	if (m_bufferevent == NULL) {
		Close();
		TcpConnectionException ex;
		ex.SetMsg("Error creating bufferevent.");
		throw ex;
	}
	bufferevent_setcb(m_bufferevent, ReadDataCallback, NULL, ErrorCallback, this);

	/* connect to the server */
	int ret = bufferevent_socket_connect(m_bufferevent, (struct sockaddr*)&m_server_addr, sizeof(m_server_addr));
	if (ret) {
		Close();
		TcpConnectionException ex;
		ex.SetMsg("%s connect() failed, errno %d (%s)",
				m_id.c_str(), errno, strerror(errno));
		throw ex;
	}
	bufferevent_enable(m_bufferevent, EV_READ | EV_WRITE | EV_PERSIST);
}

void TcpConnection::ReadDataCallback(struct bufferevent *bufev, void* ds)
{
	reinterpret_cast<TcpConnection*>(ds)->ReadData(bufev);
}

void TcpConnection::ReadData(struct bufferevent *bufev)
{
	std::vector<unsigned char> data;
	unsigned char c;
	while (bufferevent_read(m_bufferevent, &c, 1) == 1) {
		data.push_back(c);
	}
	BaseConnection::ReadData(data);
}

void TcpConnection::ErrorCallback(struct bufferevent *bufev, short event, void* ds)
{
	reinterpret_cast<TcpConnection*>(ds)->Error(bufev, event);
}

void TcpConnection::Error(struct bufferevent *bufev, short event)
{
	if (event & BEV_EVENT_CONNECTED) {
		OpenFinished();
	} else {
		if (not m_connection_open) {
			Close();
		}
		// Notify listeners
		BaseConnection::Error(event);
	}
}

void TcpConnection::OpenFinished()
{
	m_connection_open = true;
	bufferevent_enable(m_bufferevent, EV_READ | EV_WRITE | EV_PERSIST);
	BaseConnection::OpenFinished();
}

void TcpConnection::CreateSocket()
{
	m_connect_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (m_connect_fd == -1) {
		TcpConnectionException ex;
		ex.SetMsg("%s cannot create connect socket, errno %d (%s)",
				m_id.c_str(), errno, strerror(errno));
		throw ex;
	}

	int op = 1;
	int ret = setsockopt(m_connect_fd, SOL_SOCKET, SO_REUSEADDR,
			&op, sizeof(op));
	if (ret == -1) {
		Close();
		TcpConnectionException ex;
		ex.SetMsg("%s cannot set socket options on connect socket, errno %d (%s)",
				m_id.c_str(), errno, strerror(errno));
		throw ex;
	}

	if (set_fd_nonblock(m_connect_fd)) {
		Close();
		TcpConnectionException ex;
		ex.SetMsg("%s set_fd_nonblock() failed, errno %d (%s)",
				m_id.c_str(), errno, strerror(errno));
		throw ex;
	}
}

void TcpConnection::Close()
{
	if (m_bufferevent != NULL) {
		bufferevent_free(m_bufferevent);
		m_bufferevent = NULL;
	}
	if (m_connect_fd != -1) {
		close(m_connect_fd);
		m_connect_fd = -1;
	}
	m_connection_open = false;
}

bool TcpConnection::Ready() const
{
	return m_connection_open;
}

void TcpConnection::WriteData(const void* data, size_t size)
{
	if (m_bufferevent == NULL) {
		TcpConnectionException ex;
		ex.SetMsg("Connection is currently unavailable");
		throw ex;
	}
	int ret = bufferevent_write(m_bufferevent, data, size);
	if (ret < 0) {
		TcpConnectionException ex;
		ex.SetMsg("Write data failed.");
		throw ex;
	}
}
