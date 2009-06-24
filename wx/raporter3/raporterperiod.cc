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
/* $Id$
 *
 * SZARP

 * ecto@praterm.com.pl
 */

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/valgen.h>

#include "htmlview.h"

#include "raporter.h"
#include "raporterperiod.h"


szRaporterPeriod::szRaporterPeriod(wxWindow* parent, wxWindowID id,
  const wxString& title) :
  wxDialog(parent, id, title) {

  wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
/*
  wxBoxSizer *data_sizer = new wxBoxSizer(wxHORIZONTAL);

  wxString types[] = {
    _("Actual"),
    _("Minutes"),
    _("10 Minutes"),
    _("Hours"),
    _("Day")
  };
*/
  /*
  data_sizer->Add(new wxRadioBox(this, ID_RB_PERTYPE, _("Data type"),
        wxDefaultPosition, wxDefaultSize, WXSIZEOF(types), types,
        1, wxRA_SPECIFY_COLS, wxGenericValidator(&g_data.m_type)),
      1, wxGROW | wxALL, 8);
      */

  wxStaticBoxSizer *period_sizer = new wxStaticBoxSizer(
    new wxStaticBox(this, wxID_ANY, _("Period length")), wxVERTICAL);
 
  /*
  period_sizer->Add(new wxStaticText(this, wxID_ANY, _("Hours:")), 0);
  period_sizer->Add(new wxSlider(this, ID_SL_H, -1, 0, 23,
        wxDefaultPosition, wxDefaultSize, wxSL_LABELS | wxSL_HORIZONTAL,
        wxGenericValidator(&g_data.m_per_h)),
      1, wxGROW | wxALIGN_CENTER);

  period_sizer->Add(new wxStaticText(this, wxID_ANY, _("Minutes:")), 0);
  period_sizer->Add(new wxSlider(this, ID_SL_M, -1, 0, 59,
        wxDefaultPosition, wxDefaultSize, wxSL_LABELS | wxSL_HORIZONTAL,
        wxGenericValidator(&g_data.m_per_m)),
      1, wxGROW | wxALIGN_CENTER);
  */
  period_sizer->Add(new wxStaticText(this, wxID_ANY, _("Seconds:")), 0);
  period_sizer->Add(new wxSlider(this, ID_SL_S, -1, 10, 300,
        wxDefaultPosition, wxDefaultSize, wxSL_LABELS | wxSL_HORIZONTAL,
        wxGenericValidator(&g_data.m_per_s)),
      1, wxGROW | wxALIGN_CENTER);
  
  /*
  data_sizer->Add(period_sizer, 1, wxGROW | wxALL, 8);

  top_sizer->Add(data_sizer, 1, wxALL, 8);
  */
  top_sizer->Add(period_sizer, 1, wxGROW | wxALL, 8);
  

  top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL/*|wxHELP*/), 0,
    wxALL | wxALIGN_CENTER, 8);

  this->SetSizer(top_sizer);
  top_sizer->SetSizeHints(this);

  //this->SetSize(320, 240);
}

void szRaporterPeriod::OnHelp(wxCommandEvent &ev) {
  HtmlViewerFrame *f = new HtmlViewerFrame(
    _T("/opt/szarp/resources/documentation/new/raporter/html/raporter.html"),
    this, _("Raporter - help"),
    wxDefaultPosition, wxSize(600,600));
  f->Show();
}

void szRaporterPeriod::SetPer(int hour, int min, int sec) {
  wxStaticCast(FindWindowById(ID_SL_H), wxSlider)->SetValue(hour);
  wxStaticCast(FindWindowById(ID_SL_M), wxSlider)->SetValue(min);
  wxStaticCast(FindWindowById(ID_SL_S), wxSlider)->SetValue(sec);
}

void szRaporterPeriod::OnTypeChange(wxCommandEvent &ev) {
  switch (ev.GetInt()) {
  case szPER_ACT:
    SetPer(0, 0, 10);
    break;
  case szPER_1M:
    SetPer(0, 1, 0);
    break;
  case szPER_10M:
    SetPer(0, 10, 0);
    break;
  case szPER_1H:
    SetPer(1, 0, 0);
    break;
  case szPER_1D:
    SetPer(23, 59, 59);
    break;
  }
}

IMPLEMENT_CLASS(szRaporterPeriod, wxDialog)
BEGIN_EVENT_TABLE(szRaporterPeriod, wxDialog)
	EVT_BUTTON(wxID_HELP, szRaporterPeriod::OnHelp)
	EVT_RADIOBOX(ID_RB_PERTYPE, szRaporterPeriod::OnTypeChange)
END_EVENT_TABLE()
