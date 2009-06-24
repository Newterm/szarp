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
 * Domy¶lne warto¶ci u¿ywane przez programy korzystaj±ce z IPC
 * realizowanego przez wxConnect.
 */

#ifndef __CMN_IPC_H__
#define __CMN_IPC_H__

#include <wx/string.h>

class scc_ipc_messages {
public:
	static void init_ipc();

	static const wxChar* const reload_menu_msg;
	static const wxChar* const ipc_topic;
	static const wxChar* const ok_resp;
	static const wxChar* const unknown_command_resp;
	static const wxChar* const hostname;

	static wxString scc_service;

};
#endif

