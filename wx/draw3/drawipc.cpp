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

#include "ids.h"
#include "classes.h"
#include "frmmgr.h"
#include "cfgmgr.h"
#include "drawurl.h"
#include "drawipc.h"

const wxChar* const BASE_SET_STRING = _T("BASE_STARTED");
const wxChar* const DRAW_SET_STRING = _T("DRAW_STARTED");
const wxChar* const ERROR_SET_STRING = _T("ERROR");
const wxChar* const INVALID_TOPIC_STRING = _T("INVALID_TOPIC");

DrawClient::DrawClient(wxString service) : m_service(service) {}

wxString DrawClient::SendMsg(wxString topic, wxString msg) {
	wxConnection* conn = dynamic_cast<wxConnection*>(MakeConnection(_T("localhost"), m_service, topic));

	if (conn) {
		size_t size;
		char* res = (char*)conn->Request(msg,&size);
		if (res != NULL) {
			wxString ret(res, size);
			delete conn;
			return ret;
		}
	}

	return _T("Failed to connect to running instance of draw");
}

DrawServer::DrawServer(FrameManager *frm_mgr, ConfigManager *cfg_mgr) : m_frm_mgr(frm_mgr), m_cfg_mgr(cfg_mgr) {}

wxConnectionBase *DrawServer::OnAcceptConnection(const wxString &topic) {
	if (topic != _T("START_BASE") 
			&& topic != _T("START_URL")
			&& topic != _T("START_URL_EXISTING"))
		return NULL;

	return new DrawServerConnection(m_frm_mgr, topic);
}

DrawServerConnection::DrawServerConnection(FrameManager *frm_mgr, wxString topic) : 
		m_buffer(NULL), 
		m_frm_mgr(frm_mgr),
		m_topic(topic)
{
}

wxChar* DrawServerConnection::OnRequest(const wxString& topic, const wxString& item, int *size, wxIPCFormat format) {
	if (format != wxIPC_TEXT)
		return NULL;

	const wxChar* response;
	if (topic == _T("START_BASE"))
		if (m_frm_mgr->CreateFrame(item, wxEmptyString, PERIOD_T_YEAR, -1, wxDefaultSize, wxDefaultPosition, -1, true))
			response = BASE_SET_STRING;
		else
			response = ERROR_SET_STRING;
	else if (topic == _T("START_URL") || topic == _T("START_URL_EXISTING")) {
		bool open_in_new_frame = topic == _T("START_URL_EXISTING");
		wxString prefix, window;
		PeriodType pt;
		time_t time;
		int selected_draw = -1;

		if (decode_url(item, prefix, window, pt, time, selected_draw)) {
			bool ret;
			if (open_in_new_frame)
				ret = m_frm_mgr->CreateFrame(prefix, window, pt, time, wxDefaultSize, wxDefaultPosition, selected_draw);
			else
				ret = m_frm_mgr->OpenInExistingFrame(prefix, window, pt, time, selected_draw);

			if (ret)
				response = DRAW_SET_STRING;
			else
				response = ERROR_SET_STRING;
		} else
				response = ERROR_SET_STRING;

	} else
		response = INVALID_TOPIC_STRING;

	delete m_buffer;

	int rlen = wxStrlen(response) + 1;
	if (size != NULL)  {
		*size = rlen;
		m_buffer = new wxChar[rlen];
		for (int i = 0; i < rlen; ++i)	
			m_buffer[i] = response[i];

		return m_buffer;
	} else
		return NULL;

}

DrawServerConnection::~DrawServerConnection() {
	delete m_buffer;
}
