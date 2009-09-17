/* $Id$
# SZARP: SCADA software 
# 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/valtext.h>
#include <wx/valgen.h>
#include <wx/statline.h>

#include "kontradd.h"
#include "kontroler.h"
#include "kontralarm.h"

szAlarmWindow::szAlarmWindow(szKontroler *kontroler_frame, const double& val, szProbe* probe) :
	wxFrame(kontroler_frame, 
		wxID_ANY, 
		_("Alarm"),
		wxDefaultPosition,
		wxSize(300, 200), 
		wxRESIZE_BORDER | wxFRAME_NO_TASKBAR | wxCAPTION), m_kontroler_frame(kontroler_frame), m_param_name(probe->m_parname) {

	if (probe->m_alarm_type == 4)
		SetBackgroundColour(wxColour(_T("YELLOW")));

	if (probe->m_alarm_type == 5)
		SetBackgroundColour(*wxRED);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(new wxStaticText(this, wxID_ANY, _("Alarm:")), 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 2);

	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND);

	wxBoxSizer* sizer_1 = new wxBoxSizer(wxHORIZONTAL);
	sizer_1->Add(new wxStaticText(this, wxID_ANY, _("Raise time:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT), 1);
	sizer_1->Add(new wxStaticText(this, wxID_ANY, wxDateTime::Now().Format(), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT), 1);
	sizer->Add(sizer_1, 0, wxEXPAND | wxALL, 2);

	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND);

	wxString alarm_text;

	if (std::isnan(val))
		alarm_text = wxString::Format(_("No data for param: %s"), probe->m_parname.c_str());
	else if (val < probe->m_value_min)
		alarm_text = 
			wxString::Format(_("Value of param %s is to small: %.*f%s, allowed range - (%.*f%s - %.*f%s)"), 
					probe->m_parname.c_str(), 
					probe->m_param->GetPrec(), val, probe->m_param->GetUnit().c_str(), 
					probe->m_param->GetPrec(), probe->m_value_min, probe->m_param->GetUnit().c_str(),
					probe->m_param->GetPrec(), probe->m_value_max, probe->m_param->GetUnit().c_str());

	else
		alarm_text = 
			wxString::Format(_("Value of param %s is to great: %.*f%s, allowed range - (%.*f%s - %.*f%s)"), 
					probe->m_alt_name.IsEmpty() ? probe->m_parname.c_str() : probe->m_alt_name.c_str(), 
					probe->m_param->GetPrec(), val, probe->m_param->GetUnit().c_str(),
					probe->m_param->GetPrec(), probe->m_value_min, probe->m_param->GetUnit().c_str(),
					probe->m_param->GetPrec(), probe->m_value_max, probe->m_param->GetUnit().c_str());

	sizer->Add(new wxStaticText(this, wxID_ANY, alarm_text, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND | wxALL, 2);
	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND);

	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND);

	wxButton *button = new wxButton(this, wxID_OK, _("OK"));

	if (probe->m_alarm_type == 4)
		button->SetBackgroundColour(wxColour(_T("YELLOW")));

	if (probe->m_alarm_type == 5)
		button->SetBackgroundColour(wxColour(*wxRED));


	sizer->Add(button, 0, wxALL | wxALIGN_CENTRE_HORIZONTAL, 2);

	SetSizer(sizer);
	sizer->Fit(this);

}

void szAlarmWindow::OnOK(wxCommandEvent &event) {
	m_kontroler_frame->ConfirmAlarm(m_param_name);
}

BEGIN_EVENT_TABLE(szAlarmWindow, wxFrame)
  EVT_BUTTON(wxID_OK, szAlarmWindow::OnOK)
END_EVENT_TABLE()
