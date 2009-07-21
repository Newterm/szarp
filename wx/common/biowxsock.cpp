/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#include <openssl/ssl.h>

#include "biowxsock.h"

namespace {

int SocketFree(BIO *bio) {
	return bio == NULL ? 0 : 1;
}	

long SocketControl(BIO *bio, int cmd, long num, void *ptr) {
	long ret = 0;

	switch (cmd) {
		case BIO_CTRL_RESET:
		case BIO_C_FILE_SEEK:
			ret = 0;
			break;
		case BIO_C_FILE_TELL:
		case BIO_CTRL_INFO:
			ret = 0;
			break;
		case BIO_CTRL_GET_CLOSE:
			ret = bio->shutdown;
			break;
		case BIO_CTRL_SET_CLOSE:
			ret = 0;
			break;
		case BIO_CTRL_PENDING:
		case BIO_CTRL_WPENDING:
			ret = 0;
			break;
		case BIO_CTRL_DUP:
		case BIO_CTRL_FLUSH:
			ret = 1;
			break;
		default:
			ret = 0;
			break;
	}

	return ret;
}

int SocketNew(BIO* bio) {
	bio->init = 1;
	bio->shutdown = 0;
	bio->num = 0;
	bio->ptr = NULL;
	bio->flags = 0;
	return 1;
}

int SocketWrite(BIO* bio, const char *buffer, int len) {
	wxSocketClient* socket = wxDynamicCast(bio->ptr, wxSocketClient);
	assert(socket);

	socket->Write(buffer, len);
	if (socket->Error()) {
		if (socket->LastError() == wxSOCKET_WOULDBLOCK) {
			BIO_clear_retry_flags(bio);
			BIO_set_retry_write(bio);
		}
		return -1;
	}

	return socket->LastCount();
}

static int SocketRead(BIO* bio, char *buffer, int len) {
	wxSocketClient* socket = wxDynamicCast(bio->ptr, wxSocketClient);
	assert(socket);

	socket->Read(buffer, len);
	if (socket->Error()) {
		if (socket->LastError() == wxSOCKET_WOULDBLOCK) {
			BIO_clear_retry_flags(bio);
			BIO_set_retry_read(bio);
		}
		return -1;
	}

	return socket->LastCount();
}

BIO_METHOD methods_client_socket = {
	254 /*must be larger than anything defined in bio.h*/ 
		| BIO_TYPE_SOURCE_SINK,
	"wx_socket_client",
	SocketWrite,
	SocketRead,
	NULL,
	NULL,
	SocketControl,
	SocketNew,
	SocketFree,
	NULL,
};

}

BIO* BIOSocketClientNew(wxSocketClient *socket) {
	BIO *ret = BIO_new(&methods_client_socket);
	if (ret == NULL)
		return NULL;

	ret->ptr = socket;
	return ret;
}

bool SSLInitializer::is_initialized = false;

void SSLInitializer::Initialize() {
	if (is_initialized)
		return;

	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_crypto_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	is_initialized = true;
}

SSLSocketConnection::SSLSocketConnection(const wxIPV4address& ip, wxEvtHandler *receiver) : m_ip(ip), m_receiver(receiver) {

	m_socket = NULL;

	m_write_blocked_on_read = false;
	m_read_blocked_on_write = false;

	m_ssl_error = false;

	m_connect_timer = NULL;

	m_ssl = NULL;
	m_ssl_ctx = NULL;
}

void SSLSocketConnection::Cleanup() {
	delete m_connect_timer;
	m_connect_timer = NULL;

	if (m_ssl) {
		SSL_free(m_ssl);
		m_ssl = NULL;
	}
}

void SSLSocketConnection::ConnectSSL() {
	SSLInitializer::Initialize();

	if (m_ssl_ctx == NULL)
		m_ssl_ctx = SSL_CTX_new(SSLv23_client_method());

	if (m_ssl == NULL) {
		m_ssl = SSL_new(m_ssl_ctx);	
		BIO *bio = BIOSocketClientNew(m_socket);
		SSL_set_bio(m_ssl, bio, bio);
	}

	while (true) {
		int ret = SSL_connect(m_ssl);
		if (ret == 1) {
			wxSocketEvent ev;
			ev.SetEventObject(m_socket);
			ev.m_event = wxSOCKET_CONNECTION;
			wxPostEvent(m_receiver, ev);

			m_state = CONNECTED;
			return;
		}

		int error = SSL_get_error(m_ssl, ret);

		switch (error) {
			case SSL_ERROR_SYSCALL:
				if (errno == EINTR)
					continue;
			default: {
				wxSocketEvent ev;
				ev.m_event = wxSOCKET_LOST;
				wxPostEvent(m_receiver, ev);
				Cleanup();
				break;
			}
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				return;
		}
	}
}

void SSLSocketConnection::SetIPAddress(wxIPV4address &ip) {
	m_ip = ip;
}

bool SSLSocketConnection::DoConnect() {
	if (m_socket == NULL) {
		m_socket = new wxSocketClient(wxSOCKET_NOWAIT);
		m_socket->SetNotify(wxSOCKET_INPUT_FLAG |
				wxSOCKET_OUTPUT_FLAG |
				wxSOCKET_CONNECTION_FLAG |
				wxSOCKET_LOST_FLAG);
		m_socket->SetEventHandler(*this);
		m_socket->Notify(true);
	}


	if (m_socket->Connect(m_ip, false) == true)
		return true;

	if (m_socket->Error() 
			&& m_socket->LastError() != wxSOCKET_WOULDBLOCK) {
		Cleanup();

		wxSocketEvent ev;
		ev.m_event = wxSOCKET_LOST;
		wxPostEvent(m_receiver, ev);

		return false;
	}

	m_state = CONNECTING;

	assert(m_connect_timer == NULL);
	m_connect_timer = new wxTimer(this, wxID_ANY);
	m_connect_timer->SetOwner(this);
	m_connect_timer->Start(1000 * 20, true);

	return false;
}

void SSLSocketConnection::OnTimer(wxTimerEvent &event) {

	wxSocketEvent ev;
	ev.SetEventObject(m_socket);
	ev.m_event = wxSOCKET_LOST;

	wxPostEvent(m_receiver, ev);

	Disconnect();
}

void SSLSocketConnection::Connect() {
	if (DoConnect() == true) {
		m_state = CONNECTING_SSL;
		ConnectSSL();
	}
}

void SSLSocketConnection::Disconnect() {
	Cleanup();

	if (m_socket) {
		m_socket->Destroy();
		m_socket = NULL;
	}
}

wxSocketError SSLSocketConnection::LastError() {
	if (m_ssl_error)
		return wxSOCKET_IOERR;

	return m_socket->LastError();

}

ssize_t SSLSocketConnection::Read(char* buffer, size_t size) {

	do {

		ssize_t ret = SSL_read(m_ssl, buffer, size);

		if (ret <= 0) {
			int error = SSL_get_error(m_ssl, ret);
			switch (error) {
				case SSL_ERROR_SYSCALL:
					if (errno == EINTR)
						continue;
					break;
				default:
					m_ssl_error = true;
					break;
				case SSL_ERROR_WANT_WRITE:
					m_read_blocked_on_write = true;
				case SSL_ERROR_WANT_READ:
					break;
			}

			return -1;
		}

		return ret;

	} while (true);

	return false;

}

ssize_t SSLSocketConnection::Write(char* buffer, size_t size) {

	do {

		ssize_t ret = SSL_write(m_ssl, buffer, size);
	
		if (ret <= 0) {
			int error = SSL_get_error(m_ssl, ret);
			switch (error) {
				case SSL_ERROR_SYSCALL:
					if (errno == EINTR)
						continue;
					break;
				default:
					m_ssl_error = true;
					break;
				case SSL_ERROR_WANT_READ:
					m_write_blocked_on_read = true;
				case SSL_ERROR_WANT_WRITE:
					break;
			}
	
			return -1;
		}
	
		return ret;

	} while (true);

	return false;
}

void SSLSocketConnection::OnSocketEvent(wxSocketEvent& event) {
	wxSocketNotify type = event.GetSocketEvent();
	if (type == wxSOCKET_LOST) {
		Disconnect();
		wxPostEvent(m_receiver, event);
		m_state = NOT_CONNECTED;
		return;
	}

	switch (m_state) {
		case CONNECTING:
			assert(type == wxSOCKET_CONNECTION);
			m_state = CONNECTING_SSL;
			delete m_connect_timer;
			m_connect_timer = NULL;
		case CONNECTING_SSL:
			ConnectSSL();
			break;
		case NOT_CONNECTED:
			break;
		case CONNECTED:
			if (type == wxSOCKET_INPUT && m_write_blocked_on_read) {
				type = wxSOCKET_OUTPUT;
				m_write_blocked_on_read = false;
			} else if (type == wxSOCKET_OUTPUT && m_read_blocked_on_write) {
				type = wxSOCKET_INPUT;
				m_read_blocked_on_write = false;
			}

			event.m_event = type;
			wxPostEvent(m_receiver, event);
			break;
	}

}

void SSLSocketConnection::Destroy() {
	Cleanup();
}

SSLSocketConnection::~SSLSocketConnection() {
	Cleanup();

	if (m_ssl_ctx) {
		SSL_CTX_free(m_ssl_ctx);
		m_ssl_ctx = NULL;
	}

	if (m_socket) {
		m_socket->Destroy();
		m_socket = NULL;
	}
}

BEGIN_EVENT_TABLE(SSLSocketConnection, wxEvtHandler)
    EVT_SOCKET(wxID_ANY, SSLSocketConnection::OnSocketEvent)
    EVT_TIMER(wxID_ANY, SSLSocketConnection::OnTimer)
END_EVENT_TABLE()

