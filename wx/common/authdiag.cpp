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
#include "authdiag.h"

#include <wx/statline.h>

AuthInfoDialog::AuthInfoDialog(wxWindow *parent) : wxDialog(parent, wxID_ANY, wxString(_("Authentication data:"))) {
	wxStaticText *stat_text;
	wxTextCtrl *text_ctrl;
	wxStaticLine *line;
	wxButton *button;

	wxArrayString empty_string_array;
	empty_string_array.Add(_T(""));
	wxTextValidator username_validator(wxFILTER_EXCLUDE_LIST, &m_username);
	username_validator.SetIncludes(empty_string_array);

	wxTextValidator password_validator(wxFILTER_EXCLUDE_LIST, &m_password);
	password_validator.SetIncludes(empty_string_array);

	wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);

	stat_text = new wxStaticText(this, wxID_ANY, _("Enter your username and password:"));
	sizer_1->Add(stat_text, 0, wxALIGN_CENTER | wxALL, 5);

	line = new wxStaticLine(this);
	sizer_1->Add(line, 0, wxEXPAND | wxALL, 5);

	sizer_1->AddStretchSpacer();

	wxFlexGridSizer *sizer_2 = new wxFlexGridSizer(2);
	sizer_2->AddGrowableCol(1);

	stat_text = new wxStaticText(this, wxID_ANY, _("Username:"));
	sizer_2->Add(stat_text, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 5);

	text_ctrl = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition, wxSize(200, -1), 0, username_validator);
	text_ctrl->SetFocus();
	sizer_2->Add(text_ctrl, 0, wxEXPAND | wxALL, 5);

	stat_text = new wxStaticText(this, wxID_ANY, _("Password:"), wxDefaultPosition, wxDefaultSize);
	sizer_2->Add(stat_text, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 5);

	text_ctrl = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition, wxSize(200, -1), wxTE_PASSWORD, password_validator);
	sizer_2->Add(text_ctrl, 0, wxEXPAND | wxALL, 5);

	sizer_1->Add(sizer_2, 0, wxEXPAND | wxALIGN_CENTRE_VERTICAL | wxALL, 5);

	sizer_1->AddStretchSpacer();

	line = new wxStaticLine(this);
	sizer_1->Add(line, 0, wxEXPAND | wxALL, 5);

	wxBoxSizer *sizer_3 = new wxBoxSizer(wxHORIZONTAL);

	button = new wxButton(this, wxID_OK, _("OK")); 
	sizer_3->Add(button, 0, wxALIGN_CENTER | wxALL , 5);
	
	button = new wxButton(this, wxID_CANCEL, _("Cancel")); 
	sizer_3->Add(button, 0, wxALIGN_CENTER | wxALL , 5);

	sizer_1->Add(sizer_3, 0, wxALIGN_CENTER | wxALL, 5);

	SetSizer(sizer_1);

	sizer_1->Fit(this);

}

wxString AuthInfoDialog::GetUsername() {
	return m_username;
}

wxString AuthInfoDialog::GetPassword() {
	return m_password;
}

void AuthInfoDialog::Clear() {
	m_username = _T("");
	m_password = _T("");

	TransferDataToWindow();
}
