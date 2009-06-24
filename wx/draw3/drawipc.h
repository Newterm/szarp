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
 * draw3
 * SZARP
 *
 * $Id$
 */

#ifndef __DRAWIPC_H__
#define __DRAWIPC_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/ipc.h>
#endif

class FrameManager;
class ConfigManager;

/**IPC client, used by instances that detect that other copy of program is already running.*/
class DrawClient : public wxClient {
	/**Socket address*/
	wxString m_service;
public:
	DrawClient(wxString service);
	/**Sends messages to other instance of application
	 * @param topic of message
	 * @param msg actual message content*/
	wxString SendMsg(wxString topic, wxString msg);
};

/**IPC server, used by copy of the program that is started first. Listens for request
 * from other instances.*/
class DrawServer : public wxServer {
	/**@see FrameManger*/
	FrameManager *m_frm_mgr;
	/**@see ConfigManager*/
	ConfigManager *m_cfg_mgr;
public:
	DrawServer(FrameManager *frm_mgr, ConfigManager *cfg_mgr);
	/**@return new @see DrawServerConnection*/
	virtual wxConnectionBase *OnAcceptConnection(const wxString &topic);
};

/**Handles one connection with a client*/
class DrawServerConnection : public wxConnection {
	/**Output buffer*/
	wxChar* m_buffer;
	/**@see FrameManager. Object we use to open new frames and tabs. */
	FrameManager *m_frm_mgr;
	/**Initlial connection topic*/
	wxString m_topic;
public:
	DrawServerConnection(FrameManager *frm_mgr, wxString topic);
	/**Services client's request.
	 * @param topic topic of request
	 * @param item request content
	 * @param size output param, size of response
	 * @return response to a request*/
	virtual wxChar* OnRequest(const wxString& topic, const wxString& item, int *size, wxIPCFormat format);
	virtual ~DrawServerConnection();
	
};

#endif
