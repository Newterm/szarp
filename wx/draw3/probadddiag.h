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

#ifndef __PROBERADDDIAG_H_
#define __PROBERADDDIAG_H_

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/listctrl.h>
#endif

class ProbersAddressDialog : public wxDialog {

	wxListCtrl *m_address_list;

	DatabaseManager *m_db_mgr;

	ConfigManager* m_cfg_mgr;

	std::map<wxString, std::pair<wxString, wxString> > m_addresses;

	std::map<wxString, std::pair<wxString, wxString> > m_modified_addresses;

	std::vector<wxString> m_prefixes;
public:
	ProbersAddressDialog(wxWindow *parent,
		DatabaseManager *db_mgr,
		ConfigManager *cfg_mgr,
		const std::map<wxString, std::pair<wxString, wxString> >& addresses);

	void OnOkButton(wxCommandEvent &e);

	void OnCancelButton(wxCommandEvent &e);

	void OnClose(wxCloseEvent &e);

	void OnHelpButton(wxCommandEvent &event);

	void OnListItemActivated(wxListEvent &event);

	std::map<wxString, std::pair<wxString, wxString> > GetModifiedAddresses();

	DECLARE_EVENT_TABLE()
};


#endif
