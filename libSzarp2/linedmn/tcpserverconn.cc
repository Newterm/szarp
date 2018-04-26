#include "tcpserverconn.h"

#include <arpa/inet.h>

void BaseServerConnectionHandler::WriteData(const void* data, size_t size) {
	ASSERT(!"This should not have been called!");
	throw ConnectionException("Cannot write on server socket");
}

void BaseServerConnectionHandler::SetConfiguration(const SerialPortConfiguration& serial_conf) {}

FdTcpServerConnection::FdTcpServerConnection(BaseServerConnectionHandler* _handler, struct event_base *ev_base): BaseConnection(ev_base), handler(_handler) {}

FdTcpServerConnection::~FdTcpServerConnection() {
	bufferevent_free(m_bufferevent);
	m_bufferevent = NULL;

	close(m_fd);
	m_fd = -1;
}

void FdTcpServerConnection::Init(UnitInfo* unit) {
	throw ConnectionException("Cannot init from unit! ServerConnection accepts only already opened connections!");
}

void FdTcpServerConnection::ErrorCallback(struct bufferevent *bufev, short event, void* conn) {
	if (event & BEV_EVENT_CONNECTED)
		return;
	reinterpret_cast<FdTcpServerConnection*>(conn)->Error(event);
}

void FdTcpServerConnection::ReadDataCallback(struct bufferevent *bufev, void* conn) {
	reinterpret_cast<FdTcpServerConnection*>(conn)->ReadData(bufev);
}

void FdTcpServerConnection::InitConnection(int fd) {
	m_fd = fd;
}

void FdTcpServerConnection::Open() {
	m_bufferevent = bufferevent_socket_new(m_event_base, m_fd,
			BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
	if (m_bufferevent == NULL)
		throw ConnectionException("Couldn't create bufferevent");

	bufferevent_setcb(m_bufferevent, ReadDataCallback, 
			NULL, ErrorCallback, this);

	bufferevent_enable(m_bufferevent, EV_READ | EV_WRITE | EV_PERSIST);

	OpenFinished();
}

void FdTcpServerConnection::Close() {
	handler->CloseConnection(this);
}

bool FdTcpServerConnection::Ready() const { return m_fd >= 0 && m_bufferevent != NULL; }

void FdTcpServerConnection::SetConfiguration(const SerialPortConfiguration& serial_conf) {
	SetConfigurationFinished();
}

void FdTcpServerConnection::ReadData(struct bufferevent *bufev)
{
	std::vector<unsigned char> data;
	unsigned char c;
	while (bufferevent_read(bufev, &c, 1) == 1) {
		data.push_back(c);
	}

	BaseConnection::ReadData(data);
}

void FdTcpServerConnection::WriteData(const void* data, size_t size) {
	if (m_bufferevent == NULL) {
		throw ConnectionException("Connection is currently unavailable");
	}
	int ret = bufferevent_write(m_bufferevent, data, size);
	if (ret < 0) {
		throw ConnectionException("Write data failed.");
	}
}

boost::optional<int> TcpServerConnectionHandler::accept_socket(int in_fd) {
	struct sockaddr_in addr;
	socklen_t size = sizeof(addr);

	int fd = -1;
	do {
		fd = accept(in_fd, (struct sockaddr*) &addr, &size);
		if (fd != -1) break;
		if (errno == EINTR)
			continue;
		else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ECONNABORTED) 
			return boost::none;
		else
			return boost::none;
	} while (true);

	if (!ip_is_allowed(addr)) {
		close(fd);
		return boost::none;
	}

	return fd;
}


int TcpServerConnectionHandler::start_listening() {
	m_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (m_fd == -1) {
		return -1;
	}

	int on = 1;
	if(-1 == setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		Close();
		return -1;
	}

	if (-1 == bind(m_fd, (struct sockaddr *)&m_addr, sizeof(m_addr))) {
		Close();
		return -1;
	}

	if (-1 == listen(m_fd, 1)) {
		Close();
		return -1;
	}

	return m_fd;
}

void TcpServerConnectionHandler::AcceptConnection(int _fd) {
	std::unique_ptr<FdTcpServerConnection> conn(new FdTcpServerConnection(this, m_event_base));

	if (auto s = accept_socket(_fd)) {
		conn->InitConnection(*s);
		conn->AddListener(this);

		try {
			conn->Open();
			m_connections.push_back(std::move(conn));
		} catch(...) {}
	}
}
void TcpServerConnectionHandler::connection_accepted_cb(int _fd, short event, void* _handler) {
	reinterpret_cast<TcpServerConnectionHandler*>(_handler)->AcceptConnection(_fd);
}

void TcpServerConnectionHandler::OpenFinished(const BaseConnection *conn) {}

void TcpServerConnectionHandler::SetConfigurationFinished(const BaseConnection *conn) {}


bool TcpServerConnectionHandler::ip_is_allowed(struct sockaddr_in in_s_addr) const {
	return m_allowed_ips.size() == 0 || m_allowed_ips.count(in_s_addr.sin_addr.s_addr) != 0;
}

void TcpServerConnectionHandler::ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data) {
	for (auto* listener : m_listeners) {
		listener->ReadData(conn, data);
	}
}

void TcpServerConnectionHandler::ReadError(const BaseConnection *conn, short int event) {
	CloseConnection(conn);
}

void TcpServerConnectionHandler::CloseConnection(const BaseConnection* conn) {
	auto _conn = const_cast<BaseConnection*>(conn);
	m_connections.erase(std::remove_if(m_connections.begin(), m_connections.end(), [_conn](const std::unique_ptr<BaseConnection>& other){ return other.get() == _conn; }));
}

void TcpServerConnectionHandler::Init(UnitInfo* unit) {
	int port;
	if (auto port_opt = unit->getOptAttribute<int>("extra:tcp-port"))
		port = *port_opt;
	else
		throw ConnectionException("No port given in unit configuration!");

	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);
	m_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ConfigureAllowedIps(unit);
}

void TcpServerConnectionHandler::ConfigureAllowedIps(UnitInfo* unit) {
	std::string ips_allowed = unit->getAttribute<std::string>("extra:tcp-allowed", "");;
	std::stringstream ss(ips_allowed);
	std::string ip_allowed;
	while (ss >> ip_allowed) {
		struct in_addr ip;
		if (inet_aton(ip_allowed.c_str(), &ip)) {
			sz_log(1, "incorrect IP address '%s'", ip_allowed.c_str());
			m_allowed_ips.insert(ip.s_addr);
		} else {
			sz_log(5, "IP address '%s' allowed", ip_allowed.c_str());
		}
	}
}

void TcpServerConnectionHandler::Open() {
	if (m_fd >= 0)
		return;
	m_fd = start_listening();
	if (m_fd < 0)
		return;
	event_set(&_event, m_fd, EV_READ | EV_PERSIST, connection_accepted_cb, this);
	event_add(&_event, NULL);
	event_base_set(m_event_base, &_event);

	BaseConnection::OpenFinished();
}

void TcpServerConnectionHandler::Close() {
	close(m_fd);
	m_fd = -1;

	for (auto& conn: m_connections)
		conn->Close();
}

bool TcpServerConnectionHandler::Ready() const {
	return m_fd != -1;
}
