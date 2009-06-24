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

#include "szastframe.h"
#include "szastconnection.h"
#include "szastapp.h"
#include "settingsdialog.h"

#include <wx/filename.h>
#include <wx/config.h>
#include <wx/dir.h>

SzastFrame::SzastFrame(wxWindow *parent, wxWindowID id) : PscFrame(parent, id) {

	m_unit = NULL;

	LoadRegulatorDefinitions();

	wxConfigBase* cfg = wxConfig::Get();

	m_path = cfg->Read(_T("port_path"), _T("/dev/ttyS0"));
	m_speed = cfg->Read(_T("speed"), _T("9600"));
	m_id = cfg->Read(_T("unit_id"), _T("1"));

	m_msggen.SetPath(m_path);
	m_msggen.SetSpeed(m_speed);
	m_msggen.SetId(m_id);

	m_awaiting = ALL;

	m_connection = new SzastConnection(this);
	m_connection->Create();
	m_connection->Run();

	EnableEditingControls(false);
}



void SzastFrame::LoadRegulatorDefinitions() {
	wxFileName resources_dir(wxGetApp().GetSzarpDir(), wxEmptyString);
	resources_dir.AppendDir(_T("resources"));
	resources_dir.AppendDir(_T("regulators"));

	wxDir dir(resources_dir.GetFullPath());	

	wxString filename;

	bool cont = dir.GetFirst(&filename, _T("*.xml"));
	while (cont) {
		wxString fn = wxFileName(resources_dir.GetFullPath(), filename).GetFullPath();
		std::vector<PscConfigurationUnit*> ut = ParsePscSystemConfiguration(fn);
		m_definitions.insert(m_definitions.end(), ut.begin(), ut.end());
		cont = dir.GetNext(&filename);
	}

#if 0
	for (std::vector<PscConfigurationUnit*>::iterator i = m_definitions.begin();
			i != m_definitions.end();
			i++) {
		wxLogError(_T("nE: %s nP:%s nL:%s nB:%s"), (*i)->GetEprom().c_str(), (*i)->GetProgram().c_str(), (*i)->GetLibrary().c_str(), (*i)->GetBuild().c_str());
	}
#endif
}

void SzastFrame::DoHandleCloseMenuItem(wxCommandEvent& event) {
	int ret = wxMessageBox(_("Do you want to close the application?"), _("Question."), wxYES_NO | wxICON_QUESTION);

	if (ret == wxYES)
		Close();
}

void SzastFrame::DoHandleReload(wxCommandEvent& event) {
	StartWaiting();

	xmlDocPtr msg = m_msggen.CreateRegulatorSettingsMessage();
	SendMsg(msg);
}

void SzastFrame::DoHandleConfigure(wxCommandEvent& event) {
	SettingsDialog* sd = new SettingsDialog(this);
	sd->SetValues(m_speed, m_path, m_id);

	if (sd->ShowModal() != wxID_OK) {
		sd->Destroy();
		return;
	}

	sd->GetValues(m_speed, m_path, m_id);
	sd->Destroy();

	m_msggen.SetPath(m_path);
	m_msggen.SetSpeed(m_speed);
	m_msggen.SetId(m_id);

	wxConfigBase *cfg = wxConfig::Get();
	cfg->Write(_T("port_path"), m_path);
	cfg->Write(_T("speed"), m_speed);
	cfg->Write(_T("unit_id"), m_id);

}

void SzastFrame::DoHandleSetPacksType(PackType pack_type) {
	StartWaiting();
	xmlDocPtr msg = m_msggen.CreateSetPacksTypeMessage(pack_type);
	SendMsg(msg);
}

void SzastFrame::DoHandleSetPacks(wxCommandEvent &event) {
	StartWaiting();

	m_awaiting = PACKS;

	xmlDocPtr msg = m_msggen.CreateSetPacksMessage(GetPacksValues(), GetPacksType());
	SendMsg(msg);
}

void SzastFrame::DoHandleSetConstants(wxCommandEvent &event) {
	StartWaiting();

	m_awaiting = CONSTANTS;

	xmlDocPtr msg = m_msggen.CreateSetConstsMessage(GetConstsValues());
	SendMsg(msg);
}

void SzastFrame::DoHandleReset(wxCommandEvent &event) {
	StartWaiting();

	xmlDocPtr msg = m_msggen.CreateResetSettingsMessage();
	SendMsg(msg);
}

void SzastFrame::DoHandlePsetDResponse(bool ok, xmlDocPtr doc) {
	assert(ok);

	StopWaiting();

	wxStatusBar *sb = GetStatusBar();
	PscRegulatorData prd;
	bool parsed = prd.ParseResponse(doc);
	if (parsed == false) {
		wxMessageBox(_("Regulator communiation error. Invalid response format."), _("Error"), wxOK);
		sb->SetStatusText(_("Regulator communiation error. Invalid response format."));
		EnableEditingControls(false);
		m_awaiting = ALL;
		return;
	}

	if (!prd.ResponseOK()) {
		wxMessageBox(wxString(_("Error while performing operation:")) + prd.GetError(), _("Error"), wxOK);
		sb->SetStatusText(wxString(_("Error while performing operation:")) + prd.GetError());
		m_awaiting = ALL;
		EnableEditingControls(false);
		return;
	}

	if (m_unit == NULL 
			|| m_unit->GetEprom() != prd.GetEprom()
			|| m_unit->GetProgram() != prd.GetProgram()
			|| m_unit->GetLibrary() != prd.GetLibrary()) {
		std::vector<PscConfigurationUnit*>::iterator i;

		m_awaiting = ALL;

		for (i = m_definitions.begin(); i != m_definitions.end(); i++)
			if ((*i)->GetEprom() == prd.GetEprom()
					&& (*i)->GetProgram() == prd.GetProgram()
					&& (*i)->GetLibrary() == prd.GetLibrary())
				break;

		if (i == m_definitions.end()) {
			EnableEditingControls(false);
			wxMessageBox(
				wxString::Format(_("Unable to find file with definition of regulator with settings: eprom %s, program %s, library: %s.\n Unable to continue."),
					prd.GetEprom().c_str(), prd.GetProgram().c_str(), prd.GetLibrary().c_str()),
				_("Error"));
			return;
		}

		m_unit = *i;

		SetPacksInfo(m_unit->GetPacksInfo());
		SetConstsInfo(m_unit->GetConstsInfo());
	}

	if (m_awaiting != PACKS)
		SetConstsValues(prd.GetConstsValues());

	if (m_awaiting != CONSTANTS) {
		SetPacksValues(prd.GetPacksValues());
		SetPacksType(prd.GetPacksType());
	}

	m_awaiting = ALL;
}

void SzastFrame::SendMsg(xmlDocPtr msg) {
	m_connection->Query(msg);
}

void SzastFrame::OnClose(wxCloseEvent &event) {
	if (event.CanVeto()) {
		int ret = wxMessageBox(_("Do you want to close the application?"), _("Question."), wxYES_NO | wxICON_QUESTION);
		if (ret != wxYES) {
			event.Veto();
			return;
		}
	}

	m_connection->Finish();
	m_connection->Wait();

	Destroy();

}

void SzastFrame::OnWindowCreate(wxWindowCreateEvent &event) {
}

BEGIN_EVENT_TABLE(SzastFrame, PscFrame)
	EVT_CLOSE(SzastFrame::OnClose)
	EVT_WINDOW_CREATE(SzastFrame::OnWindowCreate)
END_EVENT_TABLE()
