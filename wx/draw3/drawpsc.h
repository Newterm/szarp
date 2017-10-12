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
 * Remote Parametr Setter GUI
 * SZARP
 * 
 *
 * $Id$
 */

#ifndef _DRAWPSC_H
#define _DRAWPSC_H

#include "pscgui.h"

class DrawPscSystemConfigurationEditor {
	wxString m_address;
	wxString m_port;
	wxString m_prefix;
	std::vector<PscConfigurationUnit*> m_units;
	std::map<PscConfigurationUnit*, wxString> m_units_names;
	std::map<PscConfigurationUnit*, bool> m_units_data_flag;

	DrawPscFrame* m_frame;

	DrawPsc *m_draw_psc;
public:
	static DrawPscSystemConfigurationEditor* Create(wxString szarp_data_dir, IPKConfig *ipk, DrawPsc *psc);
	void SetSettableParams(IPKConfig *cfg);
	void SetSpeedAndUnitNames(IPKConfig *cfg);

	void Edit(wxString name = wxString());

	void SetAuthData(wxString username, wxString password, wxDateTime last_fetch);
	void GetAuthData(wxString& username, wxString& password, wxDateTime& last_fetch);

	std::vector<PscConfigurationUnit*>& GetUnits() { return m_units; }
	std::map<PscConfigurationUnit*, wxString>& GetUnitsNames() { return m_units_names; }
	bool& IsUnitDataPresent(PscConfigurationUnit *u);
	void GetConnectionInfo(wxString &address, wxString &port) { address = m_address; port = m_port; }
	wxString GetPrefix() { return m_prefix; }
	~DrawPscSystemConfigurationEditor();
};

class DrawPsc {
	wxString m_username;
	wxString m_password;
	wxDateTime m_last_fetch;
	wxString m_szarp_data_dir;

	std::map<wxString, DrawPscSystemConfigurationEditor*> m_editors;
public:
	DrawPsc(wxString szarp_data_dir);
	void SetAuthData(wxString username, wxString password, wxDateTime last_fetch);
	void GetAuthData(wxString& username, wxString& password, wxDateTime& last_fetch);
	void ConfigurationLoaded(IPKConfig *ipk);
	bool IsSettable(wxString prefix);
	void Edit(wxString prefix, wxString param = wxString());
};


class DrawPscFrame : public PscFrame {

	wxChoice *m_unit_choice;

	DrawPscSystemConfigurationEditor *m_psc;

	PSetDConnection *m_connection;

	PscConfigurationUnit *m_unit;

	MessagesGenerator m_msggen;

	int m_pending_pack;

	int m_pending_const;

	bool m_awaiting_packs;

	bool m_awaiting_consts;

	void Reload();

	bool EnsureCurrentAuthInfo();

	void SendMessage(xmlDocPtr doc);

	void UpdateMessageGenerator();

	bool FindParam(wxString param_name, PscConfigurationUnit*& unit, PscParamInfo*& ppi);

	bool SwitchToUnit(PscConfigurationUnit* unit);

	void StartEditingConst(int constant);

	void StartEditingPack(int pack);

	bool GetAuthInfo();
protected:
	virtual void DoHandlePsetDResponse(bool ok, xmlDocPtr result);

	virtual void DoHandleCloseMenuItem(wxCommandEvent& event);

	virtual void DoHandleReload(wxCommandEvent& event);

	virtual void DoHandleConfigure(wxCommandEvent& event);

	virtual void DoHandleSetPacksType(PackType pack_type);

	virtual void DoHandleSetPacks(wxCommandEvent &event);

	virtual void DoHandleSetConstants(wxCommandEvent &event);

	virtual void DoHandleSaveReport(wxCommandEvent &event);

	virtual void DoHandleGetReport(wxCommandEvent &event);

	virtual void DoHandleReset(wxCommandEvent& event);

public:
	void Edit(wxString param);

	void OnClose(wxCloseEvent &event);

	void OnUnitChoice(wxCommandEvent &event);

	DrawPscFrame(wxWindow * parent, DrawPscSystemConfigurationEditor *psc, wxWindowID id = -1);

	~DrawPscFrame();

	DECLARE_EVENT_TABLE()

};


#endif

