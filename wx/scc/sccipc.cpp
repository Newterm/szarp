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
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#include "sccipc.h"
#include "sccapp.h"

#include "ipc.h"

#include "config.h"

SCCConnection::SCCConnection(SCCApp *_app) : app(_app) {}

wxChar* SCCConnection::OnRequest(const wxString& WXUNUSED(topic),
			const wxString& item,
			int* size,
			wxIPCFormat WXUNUSED(format)) {
	if (item == scc_ipc_messages::reload_menu_msg) {
		assert(app != NULL);
		app->ReloadMenu();

		*size = wxStrlen(scc_ipc_messages::ok_resp) * sizeof(wxChar)
			+ sizeof(wxChar);
		return (wxChar*) scc_ipc_messages::ok_resp;
	} else {

		*size = wxStrlen(scc_ipc_messages::unknown_command_resp) * sizeof(wxChar)
			+ sizeof(wxChar);
		return (wxChar*) scc_ipc_messages::unknown_command_resp;
	}
}

wxChar* SCCConnection::SendReloadMenuMsg() {
	return Request(scc_ipc_messages::reload_menu_msg);
}

SCCConnection* SCCClient::Connect(const wxString &service) {
	return (SCCConnection*) this->MakeConnection(scc_ipc_messages::hostname, service, scc_ipc_messages::ipc_topic);
}

SCCServer::SCCServer(SCCApp* _app,const wxString &service) : app(_app) {
	Create(service);
};

wxConnectionBase* SCCServer::OnAcceptConnection(const wxString& topic) {
	if (topic != scc_ipc_messages::ipc_topic) 
		return NULL;

	return new SCCConnection(app);
}
