
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


void BaseSerialPort::ReadData(const std::vector<unsigned char>& data)
{
	for (auto* listener : m_listeners) {
		listener->ReadData(data);
	}
}

void BaseSerialPort::Error(short int event)
{
	for (auto* listener : m_listeners) {
		listener->ReadError(event);
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
	m_bufferevent = bufferevent_new(m_fd, ReadDataCallback, NULL, ErrorCallback, this);
	if (m_bufferevent == NULL) {
		Close();
		SerialPortException ex;
		ex.SetMsg("Error creating bufferevent.");
		throw ex;
	}
	bufferevent_enable(m_bufferevent, EV_READ | EV_WRITE | EV_PERSIST);
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
	BaseSerialPort::ReadData(data);
}

void SerialPort::Error(struct bufferevent *bufev, short event)
{
	// Notify listeners
	BaseSerialPort::Error(event);
}


SerialAdapter::SerialAdapter(bool enable_fifo, bool server_CN2500):
	m_enable_fifo(enable_fifo), m_server_CN2500(server_CN2500),
	m_data_bufferevent(NULL), m_cmd_bufferevent(NULL),
	m_serial_state(UNINITIALIZED)
{ }

SerialAdapter::~SerialAdapter()
{
	CloseConnection();
}

void SerialAdapter::InitTcp(std::string address, int data_port, int cmd_port)
{
	m_data_connection.InitTcp(address, data_port);
	m_cmd_connection.InitTcp(address, cmd_port);
}

void SerialAdapter::Connect(bool blocking)
{
	if (m_data_bufferevent != NULL) {
		return;
	}
	int data_socket_fd = 0;
	int cmd_socket_fd = 0;
	try {
		data_socket_fd = m_data_connection.Connect(blocking);
		cmd_socket_fd = m_cmd_connection.Connect(blocking);
	} catch (TcpConnectionException& ex) {
		SerialAdapterException mex;
		mex.SetMsg(ex.what());
		throw mex;
	}

	m_data_bufferevent = bufferevent_new(data_socket_fd, ReadDataCallback, NULL, ErrorCallback, this);
	if (m_data_bufferevent == NULL) {
		CloseConnection();
		SerialAdapterException ex;
		ex.SetMsg("Error creating bufferevent.");
		throw ex;
	}

	m_cmd_bufferevent = bufferevent_new(cmd_socket_fd, ReadCmdCallback, NULL, ErrorCallback, this);
	if (m_cmd_bufferevent == NULL) {
		CloseConnection();
		SerialAdapterException ex;
		ex.SetMsg("Error creating bufferevent.");
		throw ex;
	}
	bufferevent_enable(m_cmd_bufferevent, EV_READ | EV_WRITE | EV_PERSIST);
}

void SerialAdapter::CloseConnection()
{
	if (m_data_bufferevent != NULL) {
		bufferevent_free(m_data_bufferevent);
		m_data_bufferevent = NULL;
	}
	m_data_connection.CloseConnection();

	if (m_cmd_bufferevent != NULL) {
		bufferevent_free(m_cmd_bufferevent);
		m_cmd_bufferevent = NULL;
	}
	m_cmd_connection.CloseConnection();
}

void SerialAdapter::ReadDataCallback(struct bufferevent *bufev, void* ds)
{
	reinterpret_cast<SerialAdapter*>(ds)->ReadData(bufev);
}

void SerialAdapter::ReadCmdCallback(struct bufferevent *bufev, void* ds)
{
	reinterpret_cast<SerialAdapter*>(ds)->ReadCmd(bufev);
}

void SerialAdapter::ErrorCallback(struct bufferevent *bufev, short event, void* ds)
{
	reinterpret_cast<SerialAdapter*>(ds)->Error(bufev, event);
}

bool SerialAdapter::Ready() const
{
	return ((m_data_bufferevent != NULL) &&
		(m_cmd_bufferevent != NULL) &&
		(m_serial_state == READY));
}

void SerialAdapter::WriteData(const void* data, size_t size)
{
	if ((m_data_bufferevent == NULL) || (m_serial_state != READY))
	{
		SerialAdapterException ex;
		ex.SetMsg("Port is currently unavailable");
		throw ex;
	}
	int ret = bufferevent_write(m_data_bufferevent, data, size);
	if (ret < 0)
	{
		SerialAdapterException ex;
		ex.SetMsg("Write data failed.");
		throw ex;
	}
}

void SerialAdapter::WriteCmd(std::vector<unsigned char> &cmd_buffer)
{
	int ret = bufferevent_write(m_cmd_bufferevent, &(cmd_buffer[0]), cmd_buffer.size());
	if (ret < 0)
	{
		// Notify listeners that serial port is malfunctioning
		BaseSerialPort::Error(EVBUFFER_ERROR);
	}
	// we cannot flush socket bufferevents, so commands can be sent in a single packet
}
	
void SerialAdapter::ReadData(struct bufferevent *bufev)
{
	std::vector<unsigned char> data;
	unsigned char c;
	while (bufferevent_read(m_data_bufferevent, &c, 1) == 1) {
		data.push_back(c);
	}
	if (m_serial_state == READY) {
		BaseSerialPort::ReadData(data);
	}
	// else data is probably garbage
}

void SerialAdapter::ReadCmd(struct bufferevent *bufev)
{
	unsigned char c;
	std::vector<unsigned char> cmd_buf;
	while (bufferevent_read(m_cmd_bufferevent, &c, 1) == 1) {
		cmd_buf.push_back(c);
	}
	ProcessCmdResponse(cmd_buf);
}

void SerialAdapter::ProcessCmdResponse(std::vector<unsigned char> &cmd_buf)
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
			bufferevent_enable(m_data_bufferevent, EV_READ | EV_WRITE | EV_PERSIST);
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

void SerialAdapter::Error(struct bufferevent *bufev, short event)
{
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
	if (m_cmd_bufferevent == NULL)
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
	if ((m_data_bufferevent == NULL) || (m_serial_state != READY))
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


TcpConnection::TcpConnection():
	m_connect_fd(-1), m_in_progress(false), m_buffer_event(NULL)
{ }

TcpConnection::~TcpConnection()
{
	if (m_connect_fd != -1) {
		CloseConnection();
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

int TcpConnection::Connect(bool blocking)
{
	/* if EINPROGRESS, we shouldn't delete socket */
	if (m_in_progress == false) {
		CloseConnection();
		CreateSocket();
	}

	/* set blocking mode for Connect */
	bool nonblocking = !blocking;
	if (set_fd_nonblock(m_connect_fd, nonblocking)) {
		close(m_connect_fd);
		m_connect_fd = -1;
		TcpConnectionException ex;
		ex.SetMsg("%s set_fd_nonblock() failed, errno %d (%s)",
				m_id.c_str(), errno, strerror(errno));
		throw ex;
	}

	/* connect to the server */
	int ret = connect(m_connect_fd, (struct sockaddr*)&m_server_addr, sizeof(m_server_addr));
	if (ret == -1) {
		if ((errno == EINPROGRESS) || (errno == EALREADY)) {
			m_in_progress = true;		// we should wait
		} else {
			m_in_progress = false;
			close(m_connect_fd);
			m_connect_fd = -1;
		}
		TcpConnectionException ex;
		ex.SetMsg("%s connect() failed, errno %d (%s)",
				m_id.c_str(), errno, strerror(errno));
		throw ex;
	}

	/* set nonblocking mode for future communication */
	if (set_fd_nonblock(m_connect_fd)) {
		close(m_connect_fd);
		m_connect_fd = -1;
		TcpConnectionException ex;
		ex.SetMsg("%s set_fd_nonblock() failed, errno %d (%s)",
				m_id.c_str(), errno, strerror(errno));
		throw ex;
	}
	//std::cout << "TcpConnection connected to server " << m_address << ":" << m_port << std::endl;
	return m_connect_fd;
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
		close(m_connect_fd);
		m_connect_fd = -1;
		TcpConnectionException ex;
		ex.SetMsg("%s cannot set socket options on connect socket, errno %d (%s)",
				m_id.c_str(), errno, strerror(errno));
		throw ex;
	}
}

void TcpConnection::CloseConnection()
{
	//std::cout << "Closing TCP port: " << m_id << std::endl;
	close(m_connect_fd);
	m_connect_fd = -1;
	m_in_progress = false;
}
