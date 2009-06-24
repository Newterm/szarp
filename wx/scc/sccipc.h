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
 * scc - Szarp Control Center
 * SZARP
 
 * Darek Marcinkiewicz reksio@praterm.com.pl
 *
 * $Id$
 */
#ifndef __SCCIPC_H__
#define __SCCIPC_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/string.h>
#include <wx/ipc.h>
#endif

class SCCApp;

/**Connection class, represents connection, both on client and server side.
 * Server side instance of this object has a pointer to main application
 * object so that application can be notified of requests send by client*/
class SCCConnection : public wxConnection {
	SCCApp* app;
public: 
	SCCConnection(SCCApp* _app = NULL);
	/**Called upon reception of request from client
	@param topic unused "topic" of the converstaion
	@param item actual content of message
	@param size size of response
	@param format format of response - not used
	@return response to a request*/
	wxChar* OnRequest(const wxString& WXUNUSED(topic),
			const wxString& item,
			int* WXUNUSED(size),
			wxIPCFormat WXUNUSED(format));
	/**Sends a "reload menu" request to a server
	 * @return server's response*/
	wxChar* SendReloadMenuMsg();
};

	
/**Object responsible for connecting to server*/
class SCCClient : public wxClient {
	public:
	/**
	 * @param service - port number, unix domain socket path or DDE service name
	 */
	SCCConnection* Connect(const wxString& service); 
};

/**Server side connection class, opens port or creates service, listes on incoming
 * connections*/
class SCCServer : public wxServer {
	SCCApp* app;
	public:
	/**@param _app pointer to main application class
	 * @param service  port number, unix domain socket path or DDE service name
	 */
	SCCServer(SCCApp* _app, const wxString& service);
	virtual wxConnectionBase* OnAcceptConnection(const wxString& topic);
};
	
#endif // __SCCIPC_H__
