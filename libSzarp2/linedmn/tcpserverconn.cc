#include "tcpserverconn.h"

#include <arpa/inet.h>

void BaseServerConnectionHandler::WriteData(const void* data, size_t size) {
	ASSERT(!"This should not have been called!");
	throw ConnectionException("Cannot write on server socket");
}

void BaseServerConnectionHandler::SetConfiguration(const SerialPortConfiguration& serial_conf) {}

TcpServerConnectionHandler::TcpServerConnectionHandler(): BaseServerConnectionHandler() {
	m_event_base = std::make_shared<EventBase>(PEvBase(nullptr));
}

void TcpServerConnectionHandler::AcceptConnection(evutil_socket_t fd, struct sockaddr *addr)
{
	if (!ip_is_allowed((struct sockaddr_in*) addr)) {
		close(fd);
		return;
	}

	PBufferevent bufev = m_event_base->CreateBufferevent(fd, BEV_OPT_CLOSE_ON_FREE);
	std::unique_ptr<TcpBaseServerConnection> conn(new TcpBaseServerConnection(this, bufev));
	conn->AddListener(this);

	try {
		conn->Open();
		m_connections.push_back(std::move(conn));
	} catch(const std::exception& e) {
		sz_log(2, "Connection error while trying to open: %s", e.what());
	}
}

void TcpServerConnectionHandler::AcceptCallback(struct evconnlistener *listener,
	evutil_socket_t fd, struct sockaddr *address, int socklen, void *ds)
{
	reinterpret_cast<TcpServerConnectionHandler*>(ds)->AcceptConnection(fd, address);
}

void TcpServerConnectionHandler::AcceptErrorCallback(struct evconnlistener *listener, void *ds)
{
	const int error = EVUTIL_SOCKET_ERROR();	// on Unix same as errno
	switch (error) {
		// "socket is nonblock and no connections are present to be accepted."
		//case EWOULDBLOCK:	// should never happen
		case EAGAIN:		// the same as EWOULDBLOCK
		case EINTR:
		case ECONNABORTED:
			return;
		default:
			std::string error_str = evutil_socket_error_to_string(error);
			sz_log(1, "Tcp server could not open connection, reason was: %s", error_str.c_str());
			throw ConnectionException("Unable to open socket");
	}
}

void TcpServerConnectionHandler::OpenFinished(const BaseConnection *conn) {}

void TcpServerConnectionHandler::SetConfigurationFinished(const BaseConnection *conn) {}

bool TcpServerConnectionHandler::ip_is_allowed(struct sockaddr_in *in_s_addr) const {
	return m_allowed_ips.empty() || m_allowed_ips.count(in_s_addr->sin_addr.s_addr) != 0;
}

void TcpServerConnectionHandler::ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data) {
	for (auto listener: m_listeners) {
		listener->ReadData(conn, data);
	}
}

void TcpServerConnectionHandler::ReadError(const BaseConnection *conn, short int event) {
	CloseConnection(conn);
}

void TcpServerConnectionHandler::CloseConnection(const BaseConnection* conn) {
	auto _conn = conn;//const_cast<BaseConnection*>(conn);
	m_connections.erase(std::remove_if(m_connections.begin(), m_connections.end(), [_conn](const std::unique_ptr<BaseConnection>& other){ return other.get() == _conn; }));
}

void TcpServerConnectionHandler::Init(UnitInfo* unit) {
	auto l_port = unit->getAttribute<int>("extra:tcp-port");
	auto l_addr = unit->getAttribute<std::string>("extra:tcp-address", "*");
	ConfigureAllowedIps(unit);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(l_port);

	if (l_addr == "*") {
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		const int res = inet_aton("127.0.0.1", &addr.sin_addr);
		if (res == 0) {
			throw ConnectionException("Parsing of server IP address '"
					+ l_addr + "' failed.");
		}
	}

	m_listener = m_event_base->CreateListenerBind(AcceptCallback, this,
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, &addr);
	evconnlistener_set_error_cb(m_listener.get(), AcceptErrorCallback);

	std::thread([this](){ m_event_base->Dispatch(); }).detach();
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
	BaseConnection::OpenFinished();
}

void TcpServerConnectionHandler::Close() {
	for (auto& conn: m_connections)
		CloseConnection(conn.get());
}

bool TcpServerConnectionHandler::Ready() const {
	return true;
}


TcpBaseServerConnection::TcpBaseServerConnection(BaseServerConnectionHandler* _handler, PBufferevent bufev): BaseConnection(), handler(_handler), m_bufferevent(bufev)
{
	bufferevent_setcb(bufev.get(), ReadDataCallback, NULL, ErrorCallback, this);
}

bool TcpBaseServerConnection::Ready() const { return true; }

void TcpBaseServerConnection::SetConfiguration(const SerialPortConfiguration& serial_conf) {
	SetConfigurationFinished();
}

void TcpBaseServerConnection::Close() { handler->CloseConnection(this); }

void TcpBaseServerConnection::Open()
{
	//PERSIST is not needed on bufevs
	bufferevent_enable(m_bufferevent.get(), EV_READ | EV_WRITE);
}

void TcpBaseServerConnection::WriteData(const void* data, size_t size)
{
	if (!m_bufferevent) {
		throw ConnectionException("Connection is currently unavailable");
	}
	const int ret = bufferevent_write(m_bufferevent.get(), data, size);
	if (ret < 0) {
		throw ConnectionException("Write data failed.");
	}
}

void TcpBaseServerConnection::ReadDataCallback(struct bufferevent *bufev, void* ds)
{
	std::vector<unsigned char> data;
	unsigned char c;
	while (bufferevent_read(bufev, &c, 1) == 1) {
		data.push_back(c);
	}

	reinterpret_cast<TcpBaseServerConnection*>(ds)->ReadData(data);
}

void TcpBaseServerConnection::ErrorCallback(struct bufferevent *bufev, short event, void* ds)
{
	if (event & BEV_EVENT_CONNECTED) {
		reinterpret_cast<TcpBaseServerConnection*>(ds)->OpenFinished();
	} else {
		reinterpret_cast<TcpBaseServerConnection*>(ds)->Error(event);
	}
}

template <>
TcpServerConnectionHandler* BaseConnFactory::create_from_unit<TcpServerConnectionHandler, UnitInfo*>(UnitInfo* unit) {
	auto conn = new TcpServerConnectionHandler();
	conn->Init(unit);
	return conn;
}
