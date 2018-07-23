#include "serialadapter.h"
#include <linux/serial_reg.h>

const int SerialAdapter::DEFAULT_DATA_PORT;
const int SerialAdapter::DEFAULT_CMD_PORT;

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
		throw ConnectionException("Open already pending");
	}
	try {
		m_data_connection->Open();
		m_cmd_connection->Open();
		m_open_pending = true;
	} catch (const TcpConnectionException& e) {
		Close();
		throw SerialAdapterException(e.what());
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
		throw SerialAdapterException("Port is currently unavailable");
	}
	try {
		m_data_connection->WriteData(data, size);
	} catch (const TcpConnectionException& e) {
		throw SerialAdapterException(e.what());
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

void SerialAdapter::SetConfiguration(const SerialPortConfiguration& config)
{
	struct termios termios = {0};
	BaseSerialPort::SerialPortConfigurationToTermios(config, &termios);

	int32_t mode = termios.c_cflag & CSIZE;
	if (mode == CS5)
		mode = SERIAL_BITS5;
	else if (mode == CS6)
		mode = SERIAL_BITS6;
	else if (mode == CS7)
		mode = SERIAL_BITS7;
	else if (mode == CS8)
		mode = SERIAL_BITS8;

	if (termios.c_cflag & CSTOPB)
		mode |= SERIAL_STOP2;
	else
		mode |= SERIAL_STOP1;

	if (termios.c_cflag & PARENB)
	{
		if (termios.c_cflag & CMSPAR)
			if (termios.c_cflag & PARODD)
				mode |= SERIAL_MARK;
			else
				mode |= SERIAL_SPACE;
		else
			if (termios.c_cflag & PARODD)
				mode |= SERIAL_ODD;
			else
				mode |= SERIAL_EVEN;
	}
	else
		mode |= SERIAL_NONE;

	int baudrate_index;
	switch (termios.c_cflag & (CBAUD|CBAUDEX))
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
	if (baudrate_index == 0xff) {
		throw SerialAdapterException("Baudrate not supported");
	}

	std::vector<unsigned char> cmd_buffer;
	cmd_buffer.resize(10, 0);

	cmd_buffer[0] = CMD_PORT_INIT;
	cmd_buffer[1] = 8;
	cmd_buffer[2] = baudrate_index;
	cmd_buffer[3] = mode;

	// line control
	int modem_control = 0;
	if (termios.c_cflag & CBAUD)
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
	if (termios.c_cflag & CRTSCTS)
	{
		cmd_buffer[6] = 1;
		cmd_buffer[7] = 1;
	}
	else
	{
		cmd_buffer[6] = 0;
		cmd_buffer[7] = 0;
	}
	if (termios.c_iflag & IXON)
	{
		cmd_buffer[8] = 1;
	}
	else
	{
		cmd_buffer[8] = 0;
	}
	if (termios.c_iflag & IXOFF)
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
	if (not m_cmd_connection->Ready()) {
		throw SerialAdapterException("Port is currently unavailable for config");
	}
	WriteCmd(cmd_buffer);
	// data shouldn't be read from data port until new serial settings negotiations are over

	// process response (set some termios flags differently) if necessary
}

void SerialAdapter::LineControl(bool dtr, bool rts)
{
	if (not Ready()) {
		throw SerialAdapterException("Port is currently unavailable");
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

template <>
SerialAdapter* BaseConnFactory::create_from_unit(struct event_base *base, UnitInfo *unit) {
	std::unique_ptr<SerialAdapter> conn(new SerialAdapter(base));
	auto address = unit->getAttribute<std::string>("extra:tcp-ip");
	auto dataport = unit->getAttribute("extra:tcp-data-port", SerialAdapter::DEFAULT_DATA_PORT);
	auto cmdport = unit->getAttribute("extra:tcp-cmd-port", SerialAdapter::DEFAULT_CMD_PORT);
	conn->InitTcp(address, dataport, cmdport);
	return conn.release();
}
