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

#include "config.h"

#include <wx/utils.h>

#include "ipc.h"

/*
 * Domy¶lne warto¶ci u¿ywane przez programy korzystaj±ce z IPC
 * realizowanego przez wxConnect.
 */

const wxChar* const scc_ipc_messages::reload_menu_msg = _T("RELOAD_MENU");
const wxChar* const scc_ipc_messages::ipc_topic = _T("SCCIPC");
const wxChar* const scc_ipc_messages::ok_resp = _T("OK");
const wxChar* const scc_ipc_messages::unknown_command_resp = _T("UNKNOWN COMMAND");
const wxChar* const scc_ipc_messages::hostname = _T("localhost");

wxString scc_ipc_messages::scc_service;

void scc_ipc_messages::init_ipc() {
#ifdef __WXMSW__
	scc_service = wxGetUserId() + _T("-SCC");
#else
	scc_service = wxGetHomeDir() + _T("/.scc_socket");
#endif
}

