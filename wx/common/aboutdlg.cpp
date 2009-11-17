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
 * SZARP About dialog.
 *

 *
 * $Id$
 */

#include "aboutdlg.h"
#include "imagepanel.h"

#include <wx/statline.h>

szAboutDlg::szAboutDlg(wxBitmap* bitmap, wxString programName, wxString version, wxString releasedate, wxArrayString authors) :
	 wxDialog(NULL, wxID_ANY, _("SZARP About"), wxDefaultPosition, wxSize(10,10), wxTAB_TRAVERSAL | wxFRAME_NO_TASKBAR | wxCAPTION | wxSTAY_ON_TOP | wxCLOSE_BOX)
{

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	wxString text;
	
	/* logo image */
	if (bitmap) {
		ImagePanel* img = new ImagePanel(this, *bitmap);
		sizer->Add(img, 0, wxBOTTOM | wxLEFT | wxRIGHT | wxALIGN_CENTER, 5);
	}
	sizer->Add(new wxStaticText(this, wxID_ANY, _T("(C) ") + releasedate + _(" SZARP Scada"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE  | wxALIGN_CENTER), 0, wxALIGN_CENTER | wxALL, 5);

	/* version info */
	if (!programName.IsEmpty()) {
		text = programName + _(" version: ");
	} else {
		text = _("Version: ");
	}
	text += version;
	sizer->Add(new wxStaticText(this, wxID_ANY, 
				text, 
				wxDefaultPosition, 
				wxDefaultSize, 
				wxST_NO_AUTORESIZE | wxALIGN_CENTER), 
			0, wxALIGN_CENTER | wxALL, 5);

	m_textCtrl = new wxTextCtrl(this, ID_ABOUT_TEXT,
			wxEmptyString,
			wxDefaultPosition,
			wxSize(400, 260),
			wxTE_MULTILINE | wxTE_RICH | wxTE_READONLY | wxTE_AUTO_URL | wxBORDER_NONE
			);

	wxTextAttr textAttr;

	textAttr.SetAlignment(wxTEXT_ALIGNMENT_CENTER);
	textAttr.SetTextColour(*wxBLUE);

	m_textCtrl->SetDefaultStyle(textAttr);

	*m_textCtrl <<  _T("http://www.szarp.org\n");
	textAttr.SetTextColour(*wxBLACK);
	textAttr.SetAlignment(wxTEXT_ALIGNMENT_CENTER);
	m_textCtrl->SetDefaultStyle(textAttr);
	*m_textCtrl << _("This program is distributed under terms of GPLv2.");
	*m_textCtrl <<  _T("\n");
	textAttr.SetTextColour(*wxBLUE);
	textAttr.SetAlignment(wxTEXT_ALIGNMENT_CENTER);
	m_textCtrl->SetDefaultStyle(textAttr);
	*m_textCtrl << _("http://www.gnu.org/licenses/gpl-2.0.html#TOC1");
	*m_textCtrl <<  _T("\n");
	
	textAttr.SetAlignment(wxTEXT_ALIGNMENT_LEFT);
	textAttr.SetTextColour(*wxBLACK);
	m_textCtrl->SetDefaultStyle(textAttr);

	*m_textCtrl <<  _T("\n");
	*m_textCtrl <<  _("Authors:");
	*m_textCtrl <<  _T("\n");
	for(size_t i = 0; i < authors.Count(); i++) {
		*m_textCtrl << authors[i];
		if (i < authors.Count() - 1) {
			*m_textCtrl << _T(", ");
		}
	}
	m_textCtrl->ShowPosition(0);

	sizer->Add(m_textCtrl, 1, wxEXPAND | wxALL , 1);

	sizer->Add(new wxButton(this, ID_ABOUT_DLG_OK_BUTTON, _("OK")), 0, wxALL | wxALIGN_RIGHT, 10);

	SetSizer(sizer);
	sizer->SetSizeHints(this);
}

void szAboutDlg::OnOKButton(wxCommandEvent& event)
{
	EndModal(wxID_OK);
}

void szAboutDlg::OnLink(wxTextUrlEvent& event)
{
	if (!event.GetMouseEvent().LeftUp()) {
		return;
	}
	wxString link = m_textCtrl->GetValue().Mid(event.GetURLStart(), event.GetURLEnd() - event.GetURLStart());
	
#if __WXMSW__
	if (wxLaunchDefaultBrowser(link) == false)
#else
	if (wxExecute(wxString::Format(_T("xdg-open %s"), link.c_str())) == 0)
#endif
		wxMessageBox(_("I was not able to start default browser"), _("Error"), wxICON_ERROR | wxOK);
}

BEGIN_EVENT_TABLE(szAboutDlg, wxDialog)
	EVT_BUTTON(ID_ABOUT_DLG_OK_BUTTON, szAboutDlg::OnOKButton)
	EVT_TEXT_URL(ID_ABOUT_TEXT, szAboutDlg::OnLink)
END_EVENT_TABLE()
