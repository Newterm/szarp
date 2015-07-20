#include "tcpserver.h"

#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <event2/buffer.h>

TcpServer::TcpServer(PEventBase base):
	m_address(""),
	m_listen_port(0),
	m_event_base(base)
{}

void TcpServer::InitTcp(const std::string address, const int port)
{
	m_address = address;
	m_listen_port = port;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_listen_port);

	if (address == "*") {
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		const int res = inet_aton(address.c_str(), &addr.sin_addr);
		if (res == 0) {
			throw TcpServerException("Parsing of server IP address '"
					+ address + "' failed.");
		}
	}

	m_listener = m_event_base->CreateListenerBind(AcceptCallback, this,
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, &addr);
	evconnlistener_set_error_cb(m_listener.get(), AcceptErrorCallback);
}

void TcpServer::AcceptCallback(struct evconnlistener *listener,
	evutil_socket_t fd, struct sockaddr *address, int socklen,
	void *ds)
{
	reinterpret_cast<TcpServer*>(ds)->AcceptConnection(fd);
}

void TcpServer::AcceptErrorCallback(struct evconnlistener *listener,
	void *ds)
{
	reinterpret_cast<TcpServer*>(ds)->AcceptError();
}

void TcpServer::AcceptConnection(evutil_socket_t fd)
{
	PBufferevent bufev =
		m_event_base->CreateBufferevent(fd, BEV_OPT_CLOSE_ON_FREE);

	auto connection = std::make_shared<TcpServerConnection>(bufev);
	for (auto* listener : m_listeners) {
		listener->AcceptConnection(this, connection);
	}
}

void TcpServer::AcceptError()
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
			for (auto* listener : m_listeners) {
				listener->AcceptError(this, error, error_str);
			}
	}
}

TcpServerConnection::TcpServerConnection(PBufferevent bufev):
	m_bufferevent(bufev),
	m_close_on_write_finished(false)
{
	bufferevent_setcb(bufev.get(), ReadDataCallback, WriteDataCallback, ErrorCallback, this);
}

void TcpServerConnection::Enable()
{
	//PERSIST is not needed on bufevs
	bufferevent_enable(m_bufferevent.get(), EV_READ | EV_WRITE);
}

void TcpServerConnection::WriteData(const void* data, size_t size)
{
	if (!m_bufferevent) {
		throw TcpServerException("Connection is currently unavailable");
	}
	const int ret = bufferevent_write(m_bufferevent.get(), data, size);
	if (ret < 0) {
		throw TcpServerException("Write data failed.");
	}
}

void TcpServerConnection::CloseOnWriteFinished()
{
	m_close_on_write_finished = true;
}

void TcpServerConnection::WriteDataCallback(struct bufferevent *bufev, void* ds)
{
	reinterpret_cast<TcpServerConnection*>(ds)->WriteFinished(bufev);
}

void TcpServerConnection::WriteFinished(struct bufferevent *bufev)
{
	if (m_close_on_write_finished and
		evbuffer_get_length(bufferevent_get_output(bufev)) == 0)
	{
		int error_code = -1;
		std::string error_str = "EOF";
		m_bufferevent.reset();

		for (auto* listener : m_listeners) {
			listener->ReadError(this, error_code, error_str);
		}
	}
}

void TcpServerConnection::ReadDataCallback(struct bufferevent *bufev, void* ds)
{
	reinterpret_cast<TcpServerConnection*>(ds)->ReadData(bufev);
}

void TcpServerConnection::ReadData(struct bufferevent *bufev)
{
	std::vector<unsigned char> data;
	unsigned char c;
	while (bufferevent_read(m_bufferevent.get(), &c, 1) == 1) {
		data.push_back(c);
	}
	for (auto* listener : m_listeners) {
		listener->ReadData(this, data);
	}
}

void TcpServerConnection::ErrorCallback(struct bufferevent *bufev, short event, void* ds)
{
	reinterpret_cast<TcpServerConnection*>(ds)->Error(bufev, event);
}

void TcpServerConnection::Error(struct bufferevent *bufev, short event)
{
	int error_code = -1;
	std::string error_str;
	if (event & BEV_EVENT_ERROR) {
		error_code = EVUTIL_SOCKET_ERROR();
		error_str = evutil_socket_error_to_string(error_code);
	} else if (event & BEV_EVENT_EOF) {
		error_str = "EOF";
		// connection closed
		m_bufferevent.reset();
	}

	for (auto* listener : m_listeners) {
		listener->ReadError(this, error_code, error_str);
	}
}
