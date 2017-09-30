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

#include <wx/xrc/xmlres.h>

#include <vector>
#include <algorithm>

#include "classes.h"
#include "ids.h"
#include "cfgmgr.h"
#include "database.h"
#include "coobs.h"
#include "defcfg.h"
#include "dbmgr.h"
#include "probadddiag.h"

const wchar_t * const DEFAULT_PROBER_PORT = L"8090";

ProbersAddressDialog::ProbersAddressDialog(wxWindow *parent, DatabaseManager *db_mgr, ConfigManager *cfg_mgr, const std::map<wxString, std::pair<wxString, wxString> >& addresses) : m_db_mgr(db_mgr), m_cfg_mgr(cfg_mgr), m_addresses(addresses) {
	SetHelpText(_T("draw3-ext-probers-addresses"));

	wxXmlResource::Get()->LoadObject(this, parent, _T("ProbersAddressDialog"), _T("wxDialog"));

	m_address_list = XRCCTRL(*this, "ProbersAddressListCtrl", wxListCtrl);
	m_address_list->InsertColumn(0, _("Server"), wxLIST_FORMAT_LEFT, 300);
	m_address_list->InsertColumn(1, _("Prefix"), wxLIST_FORMAT_LEFT, 100);
	m_address_list->InsertColumn(2, _("Address"), wxLIST_FORMAT_LEFT, 100);

	ConfigNameHash& titles = m_cfg_mgr->GetConfigTitles();
	for (ConfigNameHash::iterator i = titles.begin();
			i != titles.end();
			i++) {
		if (i->first == DefinedDrawsSets::DEF_PREFIX)
			continue;
		m_prefixes.push_back(i->first);
	}

	std::sort(m_prefixes.begin(), m_prefixes.end());

	int row = 0;
	for (std::vector<wxString>::iterator i = m_prefixes.begin();
			i != m_prefixes.end();
			i++)  {
		m_address_list->InsertItem(row, titles[*i]);
		m_address_list->SetItem(row, 1, *i);
		wxString address;
		if (m_addresses[*i].first.Length())
			address = m_addresses[*i].first + _T(":") + m_addresses[*i].second;
		m_address_list->SetItem(row, 2, address);
		row += 1;
	}

	SetSize(700, 400);

}

void ProbersAddressDialog::OnListItemActivated(wxListEvent &event) {
	wxString prefix = m_prefixes.at(event.GetIndex());
	wxString address;

	if (m_addresses[prefix].first.Length())
			address = m_addresses[prefix].first + _T(":") + m_addresses[prefix].second;

	wxString naddress = getTextFromUser(
		_("Set address for prober server:"), 
		_("Prober server address"),
		address,
		this);

	wxString server;
	wxString port;

	if(naddress.IsEmpty()) {
		server = prefix;
		port = DEFAULT_PROBER_PORT;
	}
	else if (address == naddress) {
		return;
	}
	else if (!naddress.IsEmpty()) {
		int pos = naddress.Find(L':');
		if (pos == -1) {
			server = naddress;
			port = DEFAULT_PROBER_PORT;
		} else {
			server = naddress.Mid(0, pos);
			port = naddress.Mid(pos + 1);
		}

		if(!ValidatePort(port)) {
			port = DEFAULT_PROBER_PORT;
			wxMessageBox(_("Failed to add port, it must be 1-65535 integer, setting default port"), _("Error"),
					wxOK | wxICON_ERROR, this);
		}
	}

	m_modified_addresses[prefix] = std::make_pair(server, port);
	m_addresses[prefix] = std::make_pair(server, port);

	m_address_list->SetItem(event.GetIndex(), 2, server + _T(":") + port);
}

bool ProbersAddressDialog::ValidatePort(wxString s) {
	using boost::lexical_cast;
	using boost::bad_lexical_cast;

	try {
		int port = lexical_cast<int>(s);
		return port > 0 && port <= std::numeric_limits<uint16_t>::max();
	} catch(bad_lexical_cast &) {
		return false;
	}
}

void ProbersAddressDialog::OnHelpButton(wxCommandEvent &event) {

}

void ProbersAddressDialog::OnOkButton(wxCommandEvent &event) {
	EndModal(wxID_OK);
}

void ProbersAddressDialog::OnCancelButton(wxCommandEvent &event) {
	EndModal(wxID_CANCEL);
}

void ProbersAddressDialog::OnClose(wxCloseEvent &event) {
	EndModal(wxID_CANCEL);

}

std::map<wxString, std::pair<wxString, wxString> > ProbersAddressDialog::GetModifiedAddresses() {
	return m_modified_addresses;
}

wxString ProbersAddressDialog::getTextFromUser(const wxString& message, const wxString& caption,
		const wxString& defaultValue, wxWindow *parent,
		wxCoord x, wxCoord y, bool centre)
{
	wxString str;
	long style = wxTextEntryDialogStyle;

	if (centre) {
		style |= wxCENTRE;
	}
	else {
		style &= ~wxCENTRE;
	}

	wxTextEntryDialog dialog(parent, message, caption, defaultValue, style, wxPoint(x, y));

	switch(dialog.ShowModal()) {
	case wxID_OK:
		str = dialog.GetValue();
		break;
	default:
		str = defaultValue;
		break;
	}

	return str;
}

BEGIN_EVENT_TABLE(ProbersAddressDialog, wxDialog)
	EVT_BUTTON(wxID_OK, ProbersAddressDialog::OnOkButton)
	EVT_BUTTON(wxID_HELP, ProbersAddressDialog::OnHelpButton)
	EVT_BUTTON(wxID_CANCEL, ProbersAddressDialog::OnCancelButton)
	EVT_CLOSE(ProbersAddressDialog::OnClose)
	EVT_LIST_ITEM_SELECTED(XRCID("ProbersAddressListCtrl"), ProbersAddressDialog::OnListItemActivated)
	EVT_LIST_ITEM_ACTIVATED(XRCID("ProbersAddressListCtrl"), ProbersAddressDialog::OnListItemActivated)
END_EVENT_TABLE()
