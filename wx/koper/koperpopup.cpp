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
#include <szbase/szbdefines.h>

#include "koperpopup.h"

#include "cconv.h"

BEGIN_EVENT_TABLE(KoperPopup, wxDialog)
	EVT_LEAVE_WINDOW(KoperPopup::OnMouseLeave)
	EVT_LEFT_DOWN(KoperPopup::OnMouseClick)
	EVT_MIDDLE_DOWN(KoperPopup::OnMouseClick)
	EVT_RIGHT_DOWN(KoperPopup::OnMouseClick)
END_EVENT_TABLE()


class ClickStaticText : public wxStaticText {
public:
	ClickStaticText(wxWindow *parent, 
			wxWindowID id, 
			const wxString &label, 
			const wxPoint &pos, 
			const wxSize &size , 
			int style, 
			const wxString& name = _T("ClickStaticText"));

	virtual ~ClickStaticText();
	void OnMouseClick(wxMouseEvent& event);
	DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(ClickStaticText, wxStaticText)
	EVT_LEFT_DOWN(ClickStaticText::OnMouseClick)
	EVT_MIDDLE_DOWN(ClickStaticText::OnMouseClick)
	EVT_RIGHT_DOWN(ClickStaticText::OnMouseClick)
END_EVENT_TABLE()


ClickStaticText::ClickStaticText(wxWindow *parent, 
		wxWindowID id, 
		const wxString &label, 
		const wxPoint &pos , 
		const wxSize &size , 
		int style,
		const wxString& name) :
	wxStaticText(parent, id, label, pos, size, style|wxPOPUP_WINDOW, name) 
{}

ClickStaticText::~ClickStaticText() {}

void ClickStaticText::OnMouseClick(wxMouseEvent& event) {
	wxPostEvent(GetParent(), event);
	event.Skip();
}

KoperPopup::KoperPopup(wxWindow *parent, const wxString& prefix, int value_prec) : 
	wxDialog(parent, wxID_ANY, _T("KoPer"), wxDefaultPosition, wxSize(300, 150), wxNO_BORDER | wxFRAME_FLOAT_ON_PARENT)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	ClickStaticText *s = new ClickStaticText(this, wxID_ANY, prefix.Upper(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxALL); 
	sizer->Add(s, 1, wxALIGN_CENTER | wxALL, 5);

	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND, 10);

	s = new ClickStaticText(this, wxID_ANY, _("Savings:"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxALL); 
	sizer->Add(s, 1, wxEXPAND | wxALL, 5);
	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND, 10);

	m_valstr1 = new ClickStaticText(this, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxALL); 
	sizer->Add(m_valstr1, 1, wxALIGN_CENTER | wxALL, 1);

	m_datestr1 = new ClickStaticText(this, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxALL);
	sizer->Add(m_datestr1, 1, wxALIGN_CENTER | wxALL, 1);

	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND, 2);

	m_valstr2 = new ClickStaticText(this, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxALL); 
	sizer->Add(m_valstr2, 1, wxALIGN_CENTER | wxALL, 1);

	m_datestr2 = new ClickStaticText(this, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxALL); 
	sizer->Add(m_datestr2, 1, wxALIGN_CENTER | wxALL, 1);

	m_value_prec = value_prec;

	SetSizer(sizer);
}

void KoperPopup::SetValues(double v1, const struct tm& t1, double v2, const struct tm& t2) {
	wxDateTime dt1(t1);
	wxDateTime dt2(t2);

	if (!IS_SZB_NODATA(v1))
		m_valstr1->SetLabel(wxString::Format(_T("%.*f"), m_value_prec, v1) + _(" zloty"));
	else
		m_valstr1->SetLabel(_("data missing"));

	if (!IS_SZB_NODATA(v2))
		m_valstr2->SetLabel(wxString::Format(_T("%.*f"), m_value_prec, v2) + _(" zloty"));
	else
		m_valstr2->SetLabel(_("data missing"));

	m_datestr1->SetLabel(wxString(_("counting from:")) + _T(" ") + dt1.Format(_T("%Y %b %d, %H:%M")));
	m_datestr2->SetLabel(wxString(_("counting from:")) + _T(" ") + dt2.Format(_T("%Y %b %d, %H:%M")));

}

void KoperPopup::OnMouseClick(wxMouseEvent& WXUNUSED(event)) {
	Hide();
}

void KoperPopup::OnMouseLeave(wxMouseEvent& WXUNUSED(event)) {
	Hide();
}
