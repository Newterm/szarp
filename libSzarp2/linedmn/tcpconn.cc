#include "tcpconn.h"
#include "daemonutils.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "szarp_config.h"

TcpConnection::TcpConnection(struct event_base* base)
	:BaseConnection(), m_event_base(base),
	m_connect_fd(-1), m_bufferevent(NULL), m_connection_open(false)
{}

TcpConnection::~TcpConnection()
{
	if (m_connect_fd != -1) {
		Close();
	}
}

void TcpConnection::InitTcp(const std::string& address, int port)
{
	m_address = address;
	m_port = port;
	m_id = address + ":" + std::to_string(m_port);

	/* server address */
	m_server_addr.sin_family = AF_INET;
	inet_aton(address.c_str(), &m_server_addr.sin_addr);
	m_server_addr.sin_port = htons(port);
}

void TcpConnection::Open()
{
	if (m_connect_fd != -1) {
		throw ConnectionException("Open already pending");
	}
	CreateSocket();

	m_bufferevent = bufferevent_socket_new(m_event_base, m_connect_fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
	if (m_bufferevent == NULL) {
		Close();
		throw TcpConnectionException("Error creating bufferevent.");
	}
	bufferevent_setcb(m_bufferevent, ReadDataCallback, NULL, ErrorCallback, this);

	/* connect to the server */
	int ret = bufferevent_socket_connect(m_bufferevent, (struct sockaddr*)&m_server_addr, sizeof(m_server_addr));
	if (ret) {
		Close();
		throw TcpConnectionException(errno, m_id + " connect() failed");
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
		throw TcpConnectionException(errno,
				m_id + " cannot create connect socket");
	}

	int op = 1;
	int ret = setsockopt(m_connect_fd, SOL_SOCKET, SO_REUSEADDR,
			&op, sizeof(op));
	if (ret == -1) {
		Close();
		throw TcpConnectionException(errno,
				m_id + " cannot set socket options on connect socket");
	}

	if (set_fd_nonblock(m_connect_fd)) {
		Close();
		throw TcpConnectionException(errno, m_id + " set_fd_nonblock() failed");
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
		throw TcpConnectionException("Connection is currently unavailable");
	}
	int ret = bufferevent_write(m_bufferevent, data, size);
	if (ret < 0) {
		throw TcpConnectionException("Write data failed.");
	}
}

void TcpConnection::SetConfiguration(const SerialPortConfiguration& serial_conf)
{
	SetConfigurationFinished();
}

template <>
TcpConnection* BaseConnFactory::create_from_unit(struct event_base *base, UnitInfo *unit) {
	auto conn = new TcpConnection(base);
	auto addr = unit->getAttribute<std::string>("extra:tcp-address");
	auto port = unit->getAttribute<int>("extra:tcp-port");
	conn->InitTcp(addr, port);
	return conn;
}
