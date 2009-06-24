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

#include <wx/file.h>
#include <wx/dir.h>
#include <wx/filename.h>

#include "drawpsc.h"
#include "cfgmgr.h"
#include "cconv.h"
#include "authdiag.h"

DrawPscSystemConfigurationEditor* DrawPscSystemConfigurationEditor::Create(IPKConfig *ipk, DrawPsc *psc) {
	wxString prefix = ipk->GetPrefix();

	wxFileName cd(wxGetApp().GetSzarpDataDir(), wxEmptyString);
	cd.AppendDir(prefix);
	cd.AppendDir(_T("config"));
	wxFileName packs_file_name(cd.GetPath(), _T("packs.xml"));

	std::vector<PscConfigurationUnit*> units = ParsePscSystemConfiguration(packs_file_name.GetFullPath());
	if (units.size() == 0)
		return NULL;

	std::vector<PscConfigurationUnit*>::iterator i;
	for (i = units.begin(); i != units.end(); i++)
		if ((*i)->GetPacksInfo().size() > 0
				|| (*i)->GetConstsInfo().size() > 0)
			break;
	if (i == units.end())
		return NULL;

	TSzarpConfig* sc = ipk->GetTSzarpConfig();

	std::wstring address = sc->GetPSAdress();
	std::wstring port = sc->GetPSPort();
	if (address.empty() || port.empty())
		return NULL;

	DrawPscSystemConfigurationEditor *dsc = new DrawPscSystemConfigurationEditor();
	dsc->m_units = units;
	dsc->m_address = address;
	dsc->m_port = port;
	dsc->m_prefix = prefix;
	dsc->m_draw_psc = psc;
	dsc->m_frame = NULL;

	return dsc;

}

DrawPscSystemConfigurationEditor::~DrawPscSystemConfigurationEditor() {
	if (m_frame) {
		m_frame->Destroy();
		m_frame = NULL;
	}

	for (size_t i = 0; i < m_units.size(); ++i)
		delete m_units[i];
}

void DrawPscSystemConfigurationEditor::SetAuthData(wxString username, wxString password, wxDateTime last_fetch) {
	m_draw_psc->SetAuthData(username, password, last_fetch);
}

void DrawPscSystemConfigurationEditor::GetAuthData(wxString& username, wxString& password, wxDateTime& last_fetch) {
	m_draw_psc->GetAuthData(username, password, last_fetch);
}

void DrawPscSystemConfigurationEditor::Edit(wxString param_name) {
	if (m_frame == NULL)
		m_frame = new DrawPscFrame(NULL, this);

	m_frame->Edit(param_name);
}

void DrawPscSystemConfigurationEditor::SetSettableParams(IPKConfig *cfg) {
	TSzarpConfig *sc = cfg->GetTSzarpConfig();
	for (std::vector<PscConfigurationUnit*>::iterator i = m_units.begin();
			i != m_units.end();	
			i++) {
		std::vector<PscParamInfo*> vpi = (*i)->GetParamsInfo();

		for (std::vector<PscParamInfo*>::iterator j = vpi.begin(); j != vpi.end(); j++) {
			wxString pn = (*j)->GetParamName();
			if (pn.IsEmpty())
				continue;

			TParam *p = sc->getParamByName(pn.c_str());
			if (p != NULL)
				p->SetPSC(true);

		}

	}
}

void DrawPscSystemConfigurationEditor::SetSpeedAndUnitNames(IPKConfig *cfg) {
	TSzarpConfig *sc = cfg->GetTSzarpConfig();
	for (std::vector<PscConfigurationUnit*>::iterator i = m_units.begin();
			i != m_units.end();
			i++) {
		PscConfigurationUnit* unit = *i;
		wxString path = unit->GetPath();

		for (TDevice* d = sc->GetFirstDevice(); d != NULL; d = sc->GetNextDevice(d)) {
			if (path != d->GetPath())
				continue;

			int speed = d->GetSpeed();
			if (speed != -1)
				unit->SetSpeed(wxString::Format(_T("%d"), speed));

			TUnit *u;
			for (u = d->GetFirstRadio()->GetFirstUnit(); u != NULL && unit->GetId()[0] != wxChar(u->GetId()); u = u->GetNext());

			if (u == NULL) {
				m_units_names[unit] = _T("Unknown unit name");
				continue;
			}

			wxString un;
			if (u->GetUnitName().empty()) {
				TParam *p = u->GetFirstParam();
				if (p) {
					wxString pn = p->GetName();

					int ci = pn.Find(L':', true);
					if (ci != wxNOT_FOUND)
						un = pn.Mid(0, ci);
					else
						un = pn;

				} else
					un = _T("Unknown unit name");
			}

			m_units_names[unit] = un;

		}
	}
}

bool& DrawPscSystemConfigurationEditor::IsUnitDataPresent(PscConfigurationUnit *u) {
	if (m_units_data_flag.find(u) == m_units_data_flag.end())
		m_units_data_flag[u] = false;

	return m_units_data_flag[u];
}

void DrawPsc::ConfigurationLoaded(IPKConfig *ipk) {
	wxString prefix = ipk->GetPrefix();
	if (m_editors.find(ipk->GetPrefix()) != m_editors.end())
		delete m_editors[ipk->GetPrefix()];
	m_editors.erase(ipk->GetPrefix());

	DrawPscSystemConfigurationEditor* sc = DrawPscSystemConfigurationEditor::Create(ipk, this);
	if (sc == NULL)
		return;

	sc->SetSettableParams(ipk);
	sc->SetSpeedAndUnitNames(ipk);

	m_editors[prefix] = sc;
	
}

void DrawPsc::SetAuthData(wxString username, wxString password, wxDateTime last_fetch) {
	m_username = username;
	m_password = password;
	m_last_fetch = last_fetch;
}

void DrawPsc::GetAuthData(wxString& username, wxString& password, wxDateTime& last_fetch) {
	username = m_username;
	password = m_password;
	last_fetch = m_last_fetch;
}

bool DrawPsc::IsSettable(wxString prefix) {
	return m_editors.find(prefix) != m_editors.end();
}

void DrawPsc::Edit(wxString prefix, wxString param) {
	std::map<wxString, DrawPscSystemConfigurationEditor*>::iterator i = m_editors.find(prefix);
	assert(i != m_editors.end());

	i->second->Edit(param);
}

DrawPscFrame::DrawPscFrame(wxWindow * parent, DrawPscSystemConfigurationEditor *psc, wxWindowID id) : PscFrame(parent, id), m_psc(psc) {
	wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Choose device:"));
	sizer->Add(label, 0, wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL | wxALL, 5);
	
	m_unit_choice = new wxChoice(this, psc_CHOICE, wxDefaultPosition, wxDefaultSize, wxArrayString());
	sizer->Add(m_unit_choice, 0, wxALIGN_RIGHT | wxALL, 5);

	m_main_sizer->Insert(0, sizer, 0, wxEXPAND);
	m_main_sizer->Insert(1, new wxStaticLine(this), 0, wxEXPAND);
	m_main_sizer->Layout();

	wxString address, port;	
	m_psc->GetConnectionInfo(address, port);

	wxIPV4address ip;
	ip.Hostname(address);
	long lport;
	port.ToLong(&lport);
	ip.Service(lport);

	m_connection = new PSetDConnection(ip, this);

	for (std::map<PscConfigurationUnit*, wxString>::iterator i = m_psc->GetUnitsNames().begin(); 
			i != m_psc->GetUnitsNames().end(); 
			i++)
		m_unit_choice->Append(i->second, i->first);	

	m_unit = NULL;

	m_pending_pack = m_pending_const = -1;

	m_awaiting_packs = m_awaiting_consts = false;

}

void DrawPscFrame::DoHandleReload(wxCommandEvent &event) {
	Reload();
}

void DrawPscFrame::Reload() {
	if (!EnsureCurrentAuthInfo())
		return;
	xmlDocPtr msg = m_msggen.CreateRegulatorSettingsMessage();
	SendMessage(msg);
}

void DrawPscFrame::DoHandleSetPacksType(PackType pt) {
	if (!EnsureCurrentAuthInfo())
		return;
	xmlDocPtr msg = m_msggen.CreateSetPacksTypeMessage(pt);
	SendMessage(msg);
}

void DrawPscFrame::DoHandleSetPacks(wxCommandEvent &event) {
	if (!EnsureCurrentAuthInfo())
		return;
	xmlDocPtr msg = m_msggen.CreateSetPacksMessage(GetPacksValues(), GetPacksType());
	m_awaiting_packs = true;
	SendMessage(msg);
}

void DrawPscFrame::DoHandleSetConstants(wxCommandEvent &event) {
	if (!EnsureCurrentAuthInfo())
		return;

	xmlDocPtr msg = m_msggen.CreateSetConstsMessage(GetConstsValues());
	m_awaiting_consts = true;
	SendMessage(msg);
}

void DrawPscFrame::DoHandleReset(wxCommandEvent &event) {
	if (!EnsureCurrentAuthInfo())
		return;

	xmlDocPtr msg = m_msggen.CreateResetSettingsMessage();
	SendMessage(msg);
}

void DrawPscFrame::SendMessage(xmlDocPtr doc) {
	StartWaiting();
	m_connection->QueryServer(doc);
	xmlFreeDoc(doc);
}

void DrawPscFrame::UpdateMessageGenerator() {
	m_msggen.SetSpeed(m_unit->GetSpeed());
	m_msggen.SetPath(m_unit->GetPath());
	m_msggen.SetId(m_unit->GetId());
}

bool DrawPscFrame::FindParam(wxString param_name, PscConfigurationUnit*& unit, PscParamInfo*& ppi) {
	std::vector<PscConfigurationUnit*>& units =  m_psc->GetUnits();
	for (std::vector<PscConfigurationUnit*>::iterator i = units.begin();
			i != units.end();
			i++) {

		std::vector<PscParamInfo*> pi = (*i)->GetParamsInfo();
		for (std::vector<PscParamInfo*>::iterator j = pi.begin(); j != pi.end(); j++)
			if ((*j)->GetParamName() == param_name) {
				unit = *i;
				ppi = *j;
				return true;
			}
	}

	return false;

}

void DrawPscFrame::OnUnitChoice(wxCommandEvent &event) {
	PscConfigurationUnit* unit = (PscConfigurationUnit*) m_unit_choice->GetClientData(m_unit_choice->GetSelection());
	if (SwitchToUnit(unit)) {
		EnableEditingControls(false);
		Reload();
	}

}

bool DrawPscFrame::SwitchToUnit(PscConfigurationUnit *unit) {
	if (m_unit == unit)
		return m_psc->IsUnitDataPresent(m_unit);

	size_t i;
	for (i = 0; i < m_unit_choice->GetCount(); i++)
		if (unit == m_unit_choice->GetClientData(i))
			break;

	if (i == m_unit_choice->GetCount())
		return false;

	if (m_psc->IsUnitDataPresent(m_unit)) {
		m_unit->SetPacksValues(GetPacksValues());
		m_unit->SetConstsValues(GetConstsValues());
	}

	m_unit_choice->SetSelection(i);
	m_unit = unit;

	UpdateMessageGenerator();

	SetConstsInfo(m_unit->GetConstsInfo());
	SetPacksInfo(m_unit->GetPacksInfo());
	
	if (m_psc->IsUnitDataPresent(m_unit)) {
		EnableEditingControls(true);

		SetPacksType(m_unit->GetPacksType());
		SetPacksValues(m_unit->GetPacksValues());
		SetConstsValues(m_unit->GetConstsValues());

		return false;
	} else
		return true;


}

void DrawPscFrame::Edit(wxString param) {
	if (m_pending_pack != -1 || m_pending_pack != -1)
		return;

	Show(true);
	Raise();

	if (param.IsEmpty()) {
		if (m_unit != NULL)
			return;

		m_unit = m_psc->GetUnitsNames().begin()->first;
		SetConstsInfo(m_unit->GetConstsInfo());
		SetPacksInfo(m_unit->GetPacksInfo());
		UpdateMessageGenerator();

		m_unit_choice->SetSelection(0);

		EnableEditingControls(false);
		
		Reload();

	} else {

		PscParamInfo *pi;
		PscConfigurationUnit *unit;
		bool ret = FindParam(param, unit, pi);
		assert(ret);

		if (unit == m_unit || !SwitchToUnit(unit))
			if (pi->IsPack())
				StartEditingPack(pi->GetNumber());
			else
				StartEditingConst(pi->GetNumber());
		else {
			if (pi->IsPack())
				m_pending_pack = pi->GetNumber();
			else
				m_pending_const = pi->GetNumber();

			EnableEditingControls(false);
			Reload();
		}
	}
}

void DrawPscFrame::DoHandlePsetDResponse(bool ok, xmlDocPtr doc) {
	StopWaiting();

	wxStatusBar *sb = GetStatusBar();

	if (ok == false) {
		if (IsShown())
			wxMessageBox(_("Server communiation error."), _("Error"), wxOK);
		sb->SetStatusText(_("Server communication error."));
		EnableEditingControls(false);
		return;
	}


	PscRegulatorData prd;
	bool parsed = prd.ParseResponse(doc);
	if (parsed == false) {
		if (IsShown())
			wxMessageBox(_("Server communiation error. Invalid response format."), _("Error"), wxOK);
		sb->SetStatusText(_("Server communiation error. Invalid response format."));
		EnableEditingControls(false);
		return;
	}

	if (!prd.ResponseOK()) {
		if (IsShown())
			wxMessageBox(wxString(_("Error while performing operation:")) + prd.GetError(), _("Error"), wxOK);
		sb->SetStatusText(wxString(_("Error while performing operation:")) + prd.GetError());
		EnableEditingControls(false);
		return;
	}

	assert(m_unit);
	if (m_unit->SwallowResponse(prd) == false) {
		if (IsShown())
			wxMessageBox(_("Configuration mismatch, you are not able to set parameters. Problem may be solved by synchronization of data."), _("Error"), wxOK);
		sb->SetStatusText(_("Configuration mismatch, you are not able to set parameters. Problem may be solved by synchronization of data."));
		EnableEditingControls(false);
		m_psc->IsUnitDataPresent(m_unit) = false;
		return;
	}

	EnableEditingControls(true);

	if (m_awaiting_consts == false) {
		PscFrame::SetPacksValues(prd.GetPacksValues());
		PscFrame::SetPacksType(prd.GetPacksType());
	}
	if (m_awaiting_packs == false)
		PscFrame::SetConstsValues(prd.GetConstsValues());

	m_unit->SetPacksValues(prd.GetPacksValues());
	m_unit->SetPacksType(prd.GetPacksType());
	m_unit->SetConstsValues(prd.GetConstsValues());

	if (m_pending_pack != -1) {
		int pn = m_pending_pack;
		m_pending_pack = -1;

		StartEditingPack(pn);
	} else if (m_pending_const != -1) {
		int cn = m_pending_const;
		m_pending_const = -1;

		StartEditingConst(cn);
	}

	m_psc->IsUnitDataPresent(m_unit) = true;

	m_awaiting_packs = m_awaiting_consts = false;

}

void DrawPscFrame::StartEditingConst(int constant) {
	m_notebook->SetSelection(0);

	int row = 0;
	for (std::map<int, PscParamInfo*>::iterator i = m_unit->GetConstsInfo().begin();
			i != m_unit->GetConstsInfo().end();
		       	++i, ++row) {

		if (i->second->GetNumber() != constant) 
			continue;
		
		m_const_grid->SetGridCursor(row, 1);
		break;
	}	

}

void DrawPscFrame::StartEditingPack(int pack) {
	m_notebook->SetSelection(1);
	int row = 0;
	for (std::map<int, PscParamInfo*>::iterator i = m_unit->GetPacksInfo().begin();
			i != m_unit->GetPacksInfo().end();
		       	++i, ++row) {

		if (i->second->GetNumber() != pack) 
			continue;
		
		m_pack_grid->SelectRow(row);
		m_pack_grid->SetGridCursor(row, 0);
		if (IsShown())
			m_packs_widget->ShowModal(&GetPacksValues(),
					&GetPacksInfo(),
					i->second->GetNumber(),
				       	GetPacksType());
	}

}

bool DrawPscFrame::GetAuthInfo() {
	AuthInfoDialog* ai = new AuthInfoDialog(this);
	int ret = ai->ShowModal();

	if (ret != wxID_OK) {
		ai->Destroy();
		return false;
	}

	wxString username = ai->GetUsername();
	wxString password = ai->GetPassword();
	ai->Destroy();

	m_psc->SetAuthData(username, password, wxDateTime::Now());
	m_msggen.SetUsernamePassword(username, password);

	return true;

}

bool DrawPscFrame::EnsureCurrentAuthInfo() {
	wxString username, password;
	wxDateTime wdt;

	m_psc->GetAuthData(username, password, wdt);
	if (wdt.IsValid() == false ||
			(wxDateTime::Now() - wdt).GetMinutes() > 20)
		return GetAuthInfo();

	m_msggen.SetUsernamePassword(username, password);

	return true;
}

void DrawPscFrame::DoHandleConfigure(wxCommandEvent &event) {
	GetAuthInfo();
}

void DrawPscFrame::DoHandleCloseMenuItem(wxCommandEvent &event) {
	Show(false);
}

void DrawPscFrame::OnClose(wxCloseEvent &event) {
	if (event.CanVeto()) {
		event.Veto();
		Show(false);
	} else
		Destroy();
}

DrawPscFrame::~DrawPscFrame() {
	delete m_connection;
}

BEGIN_EVENT_TABLE(DrawPscFrame, PscFrame)
	EVT_CHOICE(psc_CHOICE, DrawPscFrame::OnUnitChoice)
	EVT_CLOSE(DrawPscFrame::OnClose)
END_EVENT_TABLE()
