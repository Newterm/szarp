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
/* 
 * BIO for wxSocketClient
 * SZARP
 * 
 *
 * $Id$
 */

#ifndef __BIOWXSOCK_H__
#define __BIOWXSOCK_H__

#include <wx/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h> 
#include <wx/socket.h>
#include <wx/timer.h>

/**Creates new BIO object that channels data through wxSocketClient*/
BIO* BIOSocketClientNew(wxSocketClient *socket);

class SSLSocketConnection : public wxEvtHandler {
	/**server's ip address*/
	wxIPV4address m_ip;
	/**object handlig socket events*/
	wxEvtHandler* m_receiver;
	/**structure respresenting connection*/
	SSL *m_ssl;
	/**SSL context associated with the connection*/
	SSL_CTX *m_ssl_ctx;
	/**wxWidgets socket*/
	wxSocketClient *m_socket;
	/**timer for checking connection timeout*/
	wxTimer *m_connect_timer;

	/**last write operation was blocked on read*/
	bool m_write_blocked_on_read;

	/**last read opeartion was bloced on write*/
	bool m_read_blocked_on_write;

	/**last error that occured is raised by ssl layer*/
	bool m_ssl_error;

	/**connects to a server
	 * @return true if operation succeeds, false if it failes or is still in progress*/
	bool DoConnect();
	/**performs ssl connection to a server*/
	void ConnectSSL();

	/**frees all resources*/
	void Cleanup();

	/**timeout timer event handler*/
	void OnTimer(wxTimerEvent &event);


	enum {
		NOT_CONNECTED, 
		CONNECTING, 
		CONNECTING_SSL,
		CONNECTED } m_state;

public:
	SSLSocketConnection(const wxIPV4address& ip, wxEvtHandler *receiver);

	/**handles socket event, react appropriately*/
	void OnSocketEvent(wxSocketEvent& event);

	void Connect();

	ssize_t Write(char* buffer, size_t size);

	ssize_t Read(char* buffer, size_t size);

	void Destroy();

	virtual ~SSLSocketConnection();

	wxSocketError LastError();

	DECLARE_EVENT_TABLE();
};


/** Class for SSL library initialization*/
class SSLInitializer {
	/** Inticates if ssl library has been intialized*/
	static bool is_initialized;
public:
	/** Initilalizes SSL library (if it has not yet been initialized*/
	static void Initialize();		
};
#endif
