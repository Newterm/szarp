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

#include "kontropt.h"


szKontrolerOpt::szKontrolerOpt(wxWindow* parent, wxWindowID id,
  const wxString& title) :
  wxDialog(parent, id, title) {

  wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
  
  wxStaticBoxSizer *refresh_sizer = new wxStaticBoxSizer(
    new wxStaticBox(this, wxID_ANY, _("Refresh period")), wxVERTICAL);

  refresh_sizer->Add(new wxStaticText(this, wxID_ANY, _("Minutes:")), 0);
  refresh_sizer->Add(new wxSlider(this, wxID_ANY, -1, 0, 60,
        wxDefaultPosition, wxDefaultSize, wxSL_LABELS | wxSL_HORIZONTAL,
        wxGenericValidator(&g_data.m_per_min)),
      1, wxGROW);
  refresh_sizer->Add(new wxStaticText(this, wxID_ANY, _("Seconds:")), 0);
  refresh_sizer->Add(new wxSlider(this, wxID_ANY, -1, 0, 59,
        wxDefaultPosition, wxDefaultSize, wxSL_LABELS | wxSL_HORIZONTAL,
        wxGenericValidator(&g_data.m_per_sec)),
      1, wxGROW);

  top_sizer->Add(refresh_sizer, 1, wxGROW | wxALL, 8);

  wxStaticBoxSizer *loglen_sizer = new wxStaticBoxSizer(
    new wxStaticBox(this, wxID_ANY, _("Buffer lenght")), wxVERTICAL);
  loglen_sizer->Add(new wxSlider(this, wxID_ANY, -1, 100, 5000,
        wxDefaultPosition, wxDefaultSize, wxSL_LABELS | wxSL_HORIZONTAL,
        wxGenericValidator(&g_data.m_log_len)),
      1, wxGROW);

  top_sizer->Add(loglen_sizer, 1, wxGROW | wxALL, 8);

  top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL/*|wxHELP*/), 0,
    wxALL | wxALIGN_CENTER, 8);

  this->SetSizer(top_sizer);
  top_sizer->SetSizeHints(this);

  //this->SetSize(wxSize(300, 400));
}

void szKontrolerOpt::OnHelp(wxCommandEvent &ev) {
  HtmlViewerFrame *f = new HtmlViewerFrame(
    _T("/opt/szarp/resources/documentation/new/kontroler/html/kontroler.html"),
    this, _("Kontroler - help"),
    wxDefaultPosition, wxSize(600,600));
  f->Show();
}

IMPLEMENT_CLASS(szKontrolerOpt, wxDialog)
BEGIN_EVENT_TABLE(szKontrolerOpt, wxDialog)
  EVT_BUTTON(wxID_HELP, szKontrolerOpt::OnHelp)
END_EVENT_TABLE()
