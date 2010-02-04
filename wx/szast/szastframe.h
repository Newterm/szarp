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

#ifndef __SZASTFRAME_H__
#define __SZASTFRAME_H__

#include "wx/wxprec.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <vector>
#include "pscgui.h"

class SzastConnection;


class SzastFrame : public PscFrame {
	std::vector<PscConfigurationUnit*> m_definitions;
	PscConfigurationUnit *m_unit;
	MessagesGenerator m_msggen;
	wxString m_path, m_speed, m_id; 

	SzastConnection *m_connection;

	enum { CONSTANTS, PACKS, CONSTANTS_PACKS, REPORT } m_awaiting;

	PscReport m_psc_report;

	void LoadRegulatorDefinitions();
protected:
	virtual void DoHandlePsetDResponse(bool ok, xmlDocPtr doc);

	virtual void DoHandleCloseMenuItem(wxCommandEvent& event);

	virtual void DoHandleReload(wxCommandEvent& event);

	virtual void DoHandleConfigure(wxCommandEvent& event);

	virtual void DoHandleSetPacksType(PackType pack_type);

	virtual void DoHandleSetPacks(wxCommandEvent &event);

	virtual void DoHandleSetConstants(wxCommandEvent &event);

	virtual void DoHandleSaveReport(wxCommandEvent& event);

	virtual void DoHandleGetReport(wxCommandEvent& event);

	virtual void DoHandleReset(wxCommandEvent& event);

	void DoHandlePacksConstansResponse(xmlDocPtr doc);

	void DoHandleReportResponse(xmlDocPtr doc);

	void SendMsg(xmlDocPtr msg);
public:
	void OnWindowCreate(wxWindowCreateEvent &event);

	void OnClose(wxCloseEvent &event);

	SzastFrame(wxWindow *parent = NULL, wxWindowID id = -1);

	DECLARE_EVENT_TABLE()

};

#endif
