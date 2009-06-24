/*
 * $Id$
 *
 * kontroler3 program
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

#include <wx/valtext.h>

//#include "tokens.h"

#include "parcalc.h"

szParCalc::szParCalc(TSzarpConfig *_ipk, 
		wxWindow * parent, wxWindowID id, const wxString & title)
	: wxDialog(parent, id, title) {
  this->ipk = _ipk;

  wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

  wxTextCtrl *display_txtc = new wxTextCtrl(this, ID_TC_DISP,
      wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE,
      wxTextValidator(wxFILTER_NONE, &g_data.m_form));
  top_sizer->Add(display_txtc, 0, wxGROW | wxALL, 8);

  wxGridSizer *pad_sizer = new wxGridSizer(5, 5, 2, 2);
  //5 row, 5 col
  //row 1
  pad_sizer->Add(new wxButton(this, ID_B_SPARAM, _("Param")));
  pad_sizer->Add(0, 0);
  pad_sizer->Add(0, 0);
  pad_sizer->Add(0, 0);
  pad_sizer->Add(new wxButton(this, ID_B_CAC, _("C")));
  //row 2
  pad_sizer->Add(0, 0);
  pad_sizer->Add(new wxButton(this, ID_B_N7, _("7")));
  pad_sizer->Add(new wxButton(this, ID_B_N8, _("8")));
  pad_sizer->Add(new wxButton(this, ID_B_N9, _("9")));
  pad_sizer->Add(new wxButton(this, ID_B_OPLUS, _("+")));
  //row 3
  pad_sizer->Add(0, 0);
  pad_sizer->Add(new wxButton(this, ID_B_N4, _("4")));
  pad_sizer->Add(new wxButton(this, ID_B_N5, _("5")));
  pad_sizer->Add(new wxButton(this, ID_B_N6, _("6")));
  pad_sizer->Add(new wxButton(this, ID_B_OMINUS, _("-")));
  //row 4
  pad_sizer->Add(0, 0);
  pad_sizer->Add(new wxButton(this, ID_B_N1, _("1")));
  pad_sizer->Add(new wxButton(this, ID_B_N2, _("2")));
  pad_sizer->Add(new wxButton(this, ID_B_N3, _("3")));
  pad_sizer->Add(new wxButton(this, ID_B_OMUL, _("*")));
  //row 5
  pad_sizer->Add(0, 0);
  pad_sizer->Add(new wxButton(this, ID_B_N0, _("0")));
  pad_sizer->Add(0, 0);
  pad_sizer->Add(0, 0);
  pad_sizer->Add(new wxButton(this, ID_B_ODIV, _("/")));

  top_sizer->Add(pad_sizer, 0, wxGROW | wxALL, 8);

  this->SetSizer(top_sizer);
  top_sizer->SetSizeHints(this);

  this->ps = new szParSelect(this->ipk,
    this, wxID_ANY, title+_("->Param"),
    false, false, false, false);
}

szParCalc::~szParCalc() {
  delete ps;
}

void szParCalc::OnButtonEntry(wxCommandEvent &ev) {
  int id = ev.GetId();
  switch (id) {
   case ID_B_N0 ... ID_B_N9:
     wxLogMessage(_T("calc: but_num: %d"), id-ID_B_N0);
     break;
   case ID_B_OPLUS:
     wxLogMessage(_T("calc: but_op: plus"));
     break;
   case ID_B_OMINUS:
     wxLogMessage(_T("calc: but_op: minus"));
     break;
   case ID_B_OMUL:
     wxLogMessage(_T("calc: but_op: mul"));
     break;
   case ID_B_ODIV:
     wxLogMessage(_T("calc: but_op: div"));
     break;
   default:
     wxLogMessage(_T("calc: but_err"));
  }
}

IMPLEMENT_DYNAMIC_CLASS(szParCalc, wxDialog)
BEGIN_EVENT_TABLE(szParCalc, wxDialog)
  EVT_COMMAND_RANGE(ID_B_N0, ID_B_ODIV,
      wxEVT_COMMAND_BUTTON_CLICKED, szParCalc::OnButtonEntry)
END_EVENT_TABLE()
