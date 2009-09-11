/* $Id$
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

 kontroler3 program
 SZARP
 ecto@praterm.com.pl
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

#include "kontradd.h"

#include "kontroler.h"
#include "cconv.h"


szKontrolerAddParam::szKontrolerAddParam(TSzarpConfig *_ipk,
    wxWindow *parent, wxWindowID id, const wxString &title) :
  wxDialog(parent, id, title) {
  this->ipk = _ipk;
  ps = NULL;
  pc = NULL;

  wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

  //****
  wxStaticBoxSizer *param_sizer = new wxStaticBoxSizer(
      new wxStaticBox(this, wxID_ANY, _("Param")), wxVERTICAL);

  //text(param_name) (val: g_data.m_probe.m_parname)
  param_sizer->Add(new wxTextCtrl(this, ID_TC_PARNAME, _T(""),
        wxDefaultPosition, wxDefaultSize, wxTE_READONLY,
        wxTextValidator(wxFILTER_NONE, &g_data.m_probe.m_parname)),
      0, wxGROW | wxALL, 8);

  wxBoxSizer *paramt_sizer = new wxBoxSizer(wxHORIZONTAL);

  //butt(param)
  paramt_sizer->Add(new wxButton(this, ID_B_PARAM, _("Param")),
      0, wxALIGN_CENTER | wxALL, 8);

#if 0
  //butt(formula)
  paramt_sizer->Add(new wxButton(this, ID_B_FORMULA, _("Formula")),
      0, wxGROW | wxALL, 8);
#endif

  param_sizer->Add(paramt_sizer, 1, wxEXPAND | wxALL, 8);
  top_sizer->Add(param_sizer, 0, wxGROW | wxALL, 8);

  //****
  wxStaticBoxSizer *conds_sizer = new wxStaticBoxSizer(
      new wxStaticBox(this, wxID_ANY, _("Conditions")), wxVERTICAL);

  //check(only_is_valid) (val: g_data.m_probe.m_value_check)
  //FIXME: (should disable rest of it)
  conds_sizer->Add(new wxCheckBox(this, ID_CB_VALID, _("Only validate"),
        wxDefaultPosition, wxDefaultSize, 0,
        wxGenericValidator(&g_data.m_probe.m_value_check)),
      0, wxGROW | wxALL, 8);

  //text#num(min) (val: m_value_min)
  conds_sizer->Add(new wxTextCtrl(this, ID_TC_VALMIN, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, 0,
        wxTextValidator(wxFILTER_NUMERIC, &m_value_min)),
      0, wxGROW | wxALL, 8);

  //text#num(max) (val: m_value_max)
  conds_sizer->Add(new wxTextCtrl(this, ID_TC_VALMAX, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, 0,
        wxTextValidator(wxFILTER_NUMERIC, &m_value_max)),
      0, wxGROW | wxALL, 8);

  wxBoxSizer *condst_sizer = new wxBoxSizer(wxHORIZONTAL);

  //select(type:act|1m|10m|1h|1d) (val: g_data.m_probe.m_data_period)
  condst_sizer->Add(new wxStaticText(this, wxID_ANY, _("Monitored value:")), 1, wxALL | wxALIGN_CENTER_VERTICAL, 8);

  wxString dtypes[] = {
    _("Actual"),
    _("Minute average"),
    _("10 Minutes average"),
    _("Hour average")
  };
  condst_sizer->Add(new wxChoice(this, ID_CH_DTYPE, 
        wxDefaultPosition, wxDefaultSize, WXSIZEOF(dtypes), dtypes, 0,
        wxGenericValidator(&g_data.m_probe.m_data_period)),
      1, wxALL | wxALIGN_CENTER_VERTICAL, 8);

  //text#num-+(prec)
#if 0
  condst_sizer->Add(new wxSpinCtrl(this, ID_SC_PREC, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP,
        0, 5, 0),
      0, wxGROW | wxALL, 8);

#endif
  conds_sizer->Add(condst_sizer, 0, wxGROW | wxALL, 8);
  top_sizer->Add(conds_sizer, 0, wxGROW | wxALL, 8);

  //***
  wxStaticBoxSizer *alarm_sizer = new wxStaticBoxSizer(
      new wxStaticBox(this, wxID_ANY, _("Alarm")), wxVERTICAL);

  wxBoxSizer *alarmt_sizer = new wxBoxSizer(wxHORIZONTAL);

  alarmt_sizer->Add(new wxStaticText(this, wxID_ANY, _("Alarm type:")), 1, wxALIGN_CENTRE_VERTICAL | wxALL, 8);

  //select(alarm:1|2|3|4y|5r) (val: g_data.m_probe.m_alarm_type)
  wxString alarms[] = {
    _("1"),
    _("2"),
    _("3"),
    _("4 (yellow)"),
    _("5 (red)")
  };
  alarmt_sizer->Add(new wxChoice(this, ID_CH_ALARM, wxDefaultPosition, wxDefaultSize, WXSIZEOF(alarms), alarms, 0), 1, wxALL | wxALIGN_CENTRE_VERTICAL, 8);

  //text#num-+(group)
#if 0
  alarmt_sizer->Add(new wxSpinCtrl(this, ID_SC_GROUP, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP,
        1, 99, 1),
      0, wxGROW | wxALL, 8);
#endif

  alarm_sizer->Add(alarmt_sizer, 0, wxGROW | wxALL, 8);
  top_sizer->Add(alarm_sizer, 0, wxGROW | wxALL, 8);

  wxStaticBoxSizer *alt_param_sizer = new wxStaticBoxSizer(
      new wxStaticBox(this, wxID_ANY, _("Alternative name of parameter")), wxVERTICAL);

  alt_param_sizer->Add(new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, 0,
        wxTextValidator(wxFILTER_NONE, &g_data.m_probe.m_alt_name)),
      0, wxEXPAND | wxALL, 8);
  top_sizer->Add(alt_param_sizer, 0, wxGROW | wxALL, 8);


  wxBoxSizer *but_sizer = new wxBoxSizer(wxHORIZONTAL);
  but_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL/*|wxHELP*/), 0,
      wxALL | wxALIGN_CENTER, 8);

  top_sizer->Add(but_sizer, 0, wxALL | wxALIGN_CENTER, 8);

  this->SetSizer(top_sizer);
  //top_sizer->SetSizeHints(this);

/*  ps = new szParSelect(this->ipk,
      this, wxID_ANY, _("Kontroler->Add->Param"),
      false, false, false, false);
 */
    ps = new szParSelect(this->ipk, this, wxID_ANY, _("Kontroler->Add->Param"),
                        TRUE, TRUE, false, false,true);
    pc = new szParCalc(this->ipk,
      this, wxID_ANY, _("Kontroler->Add->Formula"));

    top_sizer->Fit(this);
}

szKontrolerAddParam::~szKontrolerAddParam() {
  delete ps;
  delete pc;
}

void szKontrolerAddParam::OnSelectParam(wxCommandEvent &ev) {
  if ( ps->ShowModal() == wxID_OK ) {
    m_param = ps->g_data.m_param;
    wxStaticCast(FindWindowById(ID_TC_PARNAME),
        wxTextCtrl)->SetValue(wxString(m_param->GetName()));
    wxLogMessage(_T("par_add: ok (%s)"),
        wxString(m_param->GetName()).c_str());
    //TODO
  }else {
    wxLogMessage(_T("par_add: cancel"));
  }
}

void szKontrolerAddParam::OnSelectFormula(wxCommandEvent &ev) {
  if ( pc->ShowModal() == wxID_OK ) {
    wxLogMessage(_T("for_add: ok"));
    //TODO
  }else {
    wxLogMessage(_T("for_add: cancel"));
  }
}

bool szKontrolerAddParam::TransferDataToWindow() {
#if 0
  wxStaticCast(FindWindowById(ID_SC_PREC),
      wxSpinCtrl)->SetValue(g_data.m_probe.m_precision);
#endif
  wxStaticCast(FindWindowById(ID_CH_ALARM), wxSpinCtrl)->SetValue(g_data.m_probe.m_alarm_type - 1);
  m_param = g_data.m_probe.m_param;

  m_value_min.Printf(_T("%.3f"), g_data.m_probe.m_value_min);
  m_value_max.Printf(_T("%.3f"), g_data.m_probe.m_value_max);

  wxDialog::TransferDataToWindow();

  return true;
}

bool szKontrolerAddParam::TransferDataFromWindow() {
  if ( m_param == NULL )
    return false;

  g_data.m_probe.m_param = m_param;
#if 0
  g_data.m_probe.m_precision = (wxStaticCast(FindWindowById(ID_SC_PREC),
      wxSpinCtrl)->GetValue());
#endif
  wxDialog::TransferDataFromWindow();

  g_data.m_probe.m_alarm_type = (wxDynamicCast(FindWindowById(ID_CH_ALARM), wxChoice)->GetSelection()) + 1;

  m_value_min.ToDouble(&g_data.m_probe.m_value_min);
  m_value_max.ToDouble(&g_data.m_probe.m_value_max);

  return true;
}

void szKontrolerAddParam::OnValidCheck(wxCommandEvent &ev) {
  if (ev.IsChecked()) {
    FindWindowById(ID_TC_VALMIN)->Disable();
    FindWindowById(ID_TC_VALMAX)->Disable();
    FindWindowById(ID_SC_PREC)->Disable();
  } else {
    FindWindowById(ID_TC_VALMIN)->Enable();
    FindWindowById(ID_TC_VALMAX)->Enable();
    FindWindowById(ID_SC_PREC)->Enable();
  }
}

IMPLEMENT_CLASS(szKontrolerAddParam, wxDialog)
BEGIN_EVENT_TABLE(szKontrolerAddParam, wxDialog)
  EVT_BUTTON(ID_B_PARAM, szKontrolerAddParam::OnSelectParam)
  EVT_BUTTON(ID_B_FORMULA, szKontrolerAddParam::OnSelectFormula)
  EVT_CHECKBOX(ID_CB_VALID, szKontrolerAddParam::OnValidCheck)
END_EVENT_TABLE()
