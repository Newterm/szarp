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

#include "raporterbuf.h"
#include "raporter.h"

#if 0
szRaporterBuf::szRaporterBuf(wxWindow* parent, wxWindowID id,
  const wxString& title) :
  wxDialog(parent, id, title) {

  wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

  wxStaticBoxSizer *data_sizer = new wxStaticBoxSizer(
    new wxStaticBox(this, wxID_ANY, _("Capacity of report buffer")), wxVERTICAL);

  data_sizer->Add(new wxSlider(this, wxID_ANY, -1, 1, 100,
        wxDefaultPosition, wxDefaultSize, wxSL_LABELS | wxSL_HORIZONTAL,
        wxGenericValidator(&g_data.m_bufsize)),
      1, wxGROW);
  
  top_sizer->Add(data_sizer, 1, wxGROW | wxALL, 8);

  top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL/*|wxHELP*/), 0,
    wxALL | wxALIGN_CENTER, 8);

  this->SetSizer(top_sizer);
  top_sizer->SetSizeHints(this);

  //this->SetSize(wxSize(300, 400));
}

void szRaporterBuf::OnHelp(wxCommandEvent &ev) {
  HtmlViewerFrame *f = new HtmlViewerFrame(
    _T("/opt/szarp/resources/documentation/new/raporter/html/raporter.html"),
    this, _("Raporter - help"),
    wxDefaultPosition, wxSize(600,600));
  f->Show();
}

IMPLEMENT_CLASS(szRaporterBuf, wxDialog)
BEGIN_EVENT_TABLE(szRaporterBuf, wxDialog)
  EVT_BUTTON(wxID_HELP, szRaporterBuf::OnHelp)
END_EVENT_TABLE()

#endif
