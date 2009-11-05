/* SZARP: SCADA software


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
#include <list>

#include "cfgdlg.h"
#include "szapp.h"
#include "szframe.h"

#include <libxml/xmlreader.h>

enum {
	CFG_DLG_LIST,
	CFG_DLG_SEARCH_TEXT,
};

BEGIN_EVENT_TABLE(ConfigDialog, wxDialog)
	EVT_BUTTON(wxID_OK, ConfigDialog::OnOK)
	EVT_BUTTON(wxID_CANCEL, ConfigDialog::OnCancel)
	EVT_COMMAND_ENTER(CFG_DLG_LIST, ConfigDialog::OnOK)
	EVT_LISTBOX_DCLICK(CFG_DLG_LIST, ConfigDialog::OnOK)
	EVT_TEXT(CFG_DLG_SEARCH_TEXT, ConfigDialog::OnSearchText)
	EVT_CLOSE(ConfigDialog::OnClose)
END_EVENT_TABLE()

class ConfigDialogKeyboardHandler: public wxEvtHandler
{
public:
	ConfigDialogKeyboardHandler(ConfigDialog *cfg) {
		m_cfg = cfg;
	}

	void OnChar(wxKeyEvent &event) {
		if (event.GetKeyCode() == WXK_UP)
			m_cfg->SelectPreviousConfig();
		else if (event.GetKeyCode() == WXK_DOWN)
			m_cfg->SelectNextConfig();
		else if (event.GetKeyCode() == WXK_RETURN)
			m_cfg->SelectCurrentConfig();
		else
			event.Skip();
	}
private:
	ConfigDialog* m_cfg;
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ConfigDialogKeyboardHandler, wxEvtHandler)
	EVT_CHAR(ConfigDialogKeyboardHandler::OnChar)
END_EVENT_TABLE()

ConfigDialog::ConfigDialog(wxWindow *parent, const ConfigNameHash& _cfgs, const wxString _userDefinedPrefix) :
	wxDialog(parent,
			wxID_ANY,
			_("Choose configuration to load:"),
			wxDefaultPosition,
			wxSize(400, 500),
			wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER
			),
	cfgs(_cfgs),
	userDefinedPrefix(_userDefinedPrefix)
{

	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(new wxStaticText(this,
				wxID_ANY,
				_("Select configuration to load:"),
				wxDefaultPosition,
				wxDefaultSize,
				wxALIGN_CENTER),
			0, wxALIGN_CENTER | wxALL, 10);

	sizer->Add(new wxStaticLine(this,
				wxID_ANY,
				wxDefaultPosition,
				wxDefaultSize),
			0, wxEXPAND);

	wxSizer *search_sizer = new wxBoxSizer(wxHORIZONTAL);
	search_sizer->Add(new wxStaticText(this,
				wxID_ANY,
				_("Search:"),
				wxDefaultPosition,
				wxDefaultSize,
				wxALIGN_CENTER),
			0, wxALIGN_CENTER | wxALL, 5);

	m_search = new wxTextCtrl(this, CFG_DLG_SEARCH_TEXT, _T(""));
	search_sizer->Add(m_search, 1, wxALL, 5);

	sizer->Add(search_sizer, 0, wxEXPAND, 5);

	m_cfg_list = new wxListBox(this, CFG_DLG_LIST);
	sizer->Add(m_cfg_list, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);

	sizer->Add(new wxStaticLine(this,
				wxID_ANY,
				wxDefaultPosition,
				wxDefaultSize),
			0, wxEXPAND);

	wxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton *ok = new wxButton(this, wxID_OK, _("OK"));
	wxButton *cancel  = new wxButton(this, wxID_CANCEL, _("Cancel"));

	button_sizer->Add(ok, 0, wxALIGN_CENTER | wxALL, 10);
	button_sizer->Add(cancel, 0, wxALIGN_CENTER | wxALL, 10);

	sizer->Add(button_sizer, 0, wxALIGN_CENTER);

	sizer->SetVirtualSizeHints(m_cfg_list);

	SetSizer(sizer);

	if (szFrame::default_icon.IsOk()) {
		SetIcon(szFrame::default_icon);
	}

	ConfigDialogKeyboardHandler *kh = new ConfigDialogKeyboardHandler(this);
	m_search->PushEventHandler(kh);

	m_show_user_defined_set = true;

	m_search->SetFocus();

}

namespace {
	struct Item {
		wxString id;
		wxString prefix;
		bool operator<(const Item &i2) {
			return this->id.CmpNoCase(i2.id) < 0;
		}
	};
}

bool ConfigDialog::SelectDatabase(wxString &database, wxArrayString* hidden_databases)
{
	ConfigDialog *cfg_dlg = new ConfigDialog(NULL, GetConfigTitles(dynamic_cast<szApp*>(wxTheApp)->GetSzarpDataDir(), hidden_databases), _T(""));

	cfg_dlg->ShowUserDefinedSets(false);

	int ret = cfg_dlg->ShowModal();
	if (ret != wxID_OK) {
		cfg_dlg->Destroy();
		return false;
	}

	database = cfg_dlg->GetSelectedPrefix();

	cfg_dlg->Destroy();

	return true;
}

void ConfigDialog::UpdateList() {
	std::list<Item> items;

	wxString match = m_search->GetValue().Lower();

	for (ConfigNameHash::const_iterator i = cfgs.begin(); i != cfgs.end(); ++i) {
		if (m_show_user_defined_set && i->first == userDefinedPrefix)
			continue;

		if (!match.IsEmpty() && i->second.Lower().Find(match) == wxNOT_FOUND)
			continue;

		Item it;
		it.id = i->second;
		it.prefix = i->first;
		items.push_back(it);
	}

	items.sort();

	m_cfg_prefixes.Clear();

	wxArrayString ids;

	for (std::list<Item>::iterator i = items.begin();
			i != items.end();
			i++) {
		ids.Add(i->id);
		m_cfg_prefixes.Add(i->prefix);
	}

	m_cfg_list->Set(ids);

	if (items.size())
		m_cfg_list->Select(0);

}

void ConfigDialog::SelectNextConfig() {
	int i = m_cfg_list->GetSelection();
	if (i >= 0 && ++i < (int)m_cfg_list->GetCount())
		m_cfg_list->Select(i);
}

void ConfigDialog::SelectCurrentConfig() {
	int i = m_cfg_list->GetSelection();
	if (i == wxNOT_FOUND)
		EndModal(wxID_CANCEL);
	else
		EndModal(wxID_OK);


}

void ConfigDialog::SelectPreviousConfig() {
	int i = m_cfg_list->GetSelection();
	if (--i >= 0)
		m_cfg_list->Select(i);
}


void ConfigDialog::OnOK(wxCommandEvent &event) {
	SelectCurrentConfig();
}

void ConfigDialog::OnCancel(wxCommandEvent &event) {
	Show(false);
}

void ConfigDialog::OnSearchText(wxCommandEvent& event) {
	UpdateList();
}

void ConfigDialog::OnClose(wxCloseEvent& event) {
	if (event.CanVeto())  {
		Show(false);
		event.Veto();
	} else
		Destroy();
}

void ConfigDialog::ShowUserDefinedSets(bool show) {
	m_show_user_defined_set = show;
}

wxString ConfigDialog::GetSelectedPrefix() {
	int i = m_cfg_list->GetSelection();
	if (i == wxNOT_FOUND)
		return wxEmptyString;

	return m_cfg_prefixes[i];
}

wxString ConfigDialog::GetSelectedTitle() {
	int i = m_cfg_list->GetSelection();
	if (i == wxNOT_FOUND)
		return wxEmptyString;

	return m_cfg_list->GetString(i);
}

int ConfigDialog::ShowModal() {
	m_search->SetValue(_T(""));
	UpdateList();

	return wxDialog::ShowModal();
}
