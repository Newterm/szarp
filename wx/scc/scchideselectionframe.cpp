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

 * Adam Smyk asmyk@praterm.pl
 *
 * $Id: scchideselectionframe.cpp 1 2009-06-24 15:09:25Z asmyk $
 */

#include "scchideselectionframe.h"


SCCSelectionFrame::SCCSelectionFrame() : wxDialog(NULL, wxID_ANY, _("Hiding SZARP databases"),
	wxDefaultPosition, wxSize(300,400), wxTAB_TRAVERSAL | wxFRAME_NO_TASKBAR | wxRESIZE_BORDER | wxCAPTION)
{
	wxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);

	main_sizer->Add(new wxStaticText(this, wxID_ANY, _("Select databases to hide")), 0, wxALIGN_CENTER | wxALL, 20);
	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);

	LoadDatabases();
	LoadConfiguration();

	wxArrayString bases;
	for (unsigned int i = 0; i < m_databases.GetCount(); i++) {
		wxString base;
		base = m_config_titles[m_databases[i]];

		std::map<wxString, wxString>::iterator j = m_database_server_map.find(m_databases[i]);
		if (j != m_database_server_map.end())
			base += _T(" (") + j->second + _T(")");

		bases.Add(base);
	}

	m_databases_list_box = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, bases);

	for (unsigned int x = 0; x < m_hidden_databases.GetCount(); x++) {
		for (unsigned int i = 0; i < m_databases.GetCount(); i++) {
			if (!m_hidden_databases[x].Cmp(m_databases[i]))
			{
				m_databases_list_box->Check(i);
				break;
			}
		}
	}

	main_sizer->Add(m_databases_list_box, 1, wxEXPAND);

	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);

	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton* button = new wxButton(this, ID_SELECTION_FRAME_OK_BUTTON, _("OK"), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	button_sizer->Add(button, 0, wxALL, 10);

	button = new wxButton(this, ID_SELECTION_FRAME_CANCEL_BUTTON, _("Cancel"), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	button_sizer->Add(button, 0, wxALL, 10);

	main_sizer->Add(button_sizer, 0, wxALL | wxALIGN_CENTER);

	SetSizer(main_sizer);

}

void SCCSelectionFrame::StoreConfiguration() {

	wxConfigBase* config = wxConfigBase::Get(true);

	wxString storestr;
	bool first = true;

	m_hidden_databases.Clear();

	for (unsigned int i = 0; i < m_databases.GetCount(); i++)
	{
		if (m_databases_list_box->IsChecked(i))
			m_hidden_databases.Add(m_databases[i]);
	}

	for (unsigned int i = 0; i < m_hidden_databases.GetCount(); i++)
	{
		if (!first)
			storestr.Append(_T(","));
		first = false;
		storestr += (m_hidden_databases[i]);
	}
	config->Write(_T("SELECTED_DATABASES"), storestr);
	config->Flush();
}

void SCCSelectionFrame::LoadConfiguration() {
	wxString tmp;
	wxConfigBase* config = wxConfigBase::Get(true);
	if (config->Read(_T("SELECTED_DATABASES"), &tmp))
	{
		wxStringTokenizer tkz(tmp, _T(","), wxTOKEN_STRTOK);
		while (tkz.HasMoreTokens())
		{
			wxString token = tkz.GetNextToken();
			token.Trim();
			if (!token.IsEmpty())
				m_hidden_databases.Add(token);
		}
	}
}

void SCCSelectionFrame::LoadDatabases() {
	m_config_titles = GetConfigTitles(dynamic_cast<szApp*>(wxTheApp)->GetSzarpDataDir());
	for (ConfigNameHash::iterator i = m_config_titles.begin(); i != m_config_titles.end(); i++)
		m_databases.Add(i->first);

	m_databases.Sort();
}

wxArrayString SCCSelectionFrame::GetHiddenDatabases() {
	return wxArrayString(m_hidden_databases);
}

void SCCSelectionFrame::OnOKButton(wxCommandEvent& event) {
	StoreConfiguration();
	EndModal(wxID_OK);
}

void SCCSelectionFrame::OnCancelButton(wxCommandEvent& event) {
    for (unsigned int i = 0; i < m_databases.GetCount(); i++) {
        m_databases_list_box->Check(i, false);
        for (unsigned int x = 0; x < m_hidden_databases.GetCount(); x++) {
			if (!m_hidden_databases[x].Cmp(m_databases[i]))
			{
				m_databases_list_box->Check(i);
				break;
			}
		}
	}
	EndModal(wxID_CANCEL);
}

BEGIN_EVENT_TABLE(SCCSelectionFrame, wxDialog)
	EVT_BUTTON(ID_SELECTION_FRAME_OK_BUTTON, SCCSelectionFrame::OnOKButton)
	EVT_BUTTON(ID_SELECTION_FRAME_CANCEL_BUTTON, SCCSelectionFrame::OnCancelButton)
END_EVENT_TABLE()
