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
/* $Id: viszioParamadd.cpp 1 2009-11-17 21:44:30Z asmyk $
 *
 * viszio program
 * SZARP
 *
 * asmyk@praterm.com.pl
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

#include "viszioParamadd.h"
#include "cconv.h"


szViszioAddParam::szViszioAddParam(TSzarpConfig *_ipk,
                                 wxWindow *parent, wxWindowID id, const wxString &title) :
        wxDialog(parent, id, title)
{
    this->ipk = _ipk;
    ps = NULL;

    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBoxSizer *param_sizer = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, _("Parameter")), wxVERTICAL);


    param_sizer->Add(new wxTextCtrl(this, ID_TC_PARNAME, _T(""),
                                    wxDefaultPosition, wxDefaultSize, wxTE_READONLY,
                                    wxTextValidator(wxFILTER_NONE, &g_data.m_probe.m_parname)),
                     0, wxGROW | wxALL, 8);

    wxBoxSizer *paramt_sizer = new wxBoxSizer(wxHORIZONTAL);

    paramt_sizer->Add(new wxButton(this, ID_B_PARAM, _("Choose parameter")),
                      0, wxALIGN_CENTER | wxALL, 8);


    param_sizer->Add(paramt_sizer, 1, wxEXPAND | wxALL, 8);
    top_sizer->Add(param_sizer, 0, wxGROW | wxALL, 8);

    wxBoxSizer *but_sizer = new wxBoxSizer(wxHORIZONTAL);
    but_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL/*|wxHELP*/), 0,
                   wxALL | wxALIGN_CENTER, 8);

    top_sizer->Add(but_sizer, 0, wxALL | wxALIGN_CENTER, 8);


    this->SetSizer(top_sizer);
    ps = new szParSelect(this->ipk, this, wxID_ANY, _("Viszio->Add->Param"),
                         TRUE, TRUE, false, false,true);
    top_sizer->Fit(this);
}

szViszioAddParam::~szViszioAddParam()
{
    delete ps;
}

void szViszioAddParam::OnSelectParam(wxCommandEvent &ev)
{
    if ( ps->ShowModal() == wxID_OK )
    {
        m_param = ps->g_data.m_param;
        wxStaticCast(FindWindowById(ID_TC_PARNAME),
                     wxTextCtrl)->SetValue(wxString(m_param->GetName()));
        wxLogMessage(_T("par_add: ok (%s)"),
                     wxString(m_param->GetName()).c_str());
        //TODO
    }
    else
    {
        wxLogMessage(_T("par_add: cancel"));
    }
}



bool szViszioAddParam::TransferDataToWindow()
{

    m_param = g_data.m_probe.m_param;
    wxDialog::TransferDataToWindow();
    return true;
}

bool szViszioAddParam::TransferDataFromWindow()
{
    if ( m_param == NULL )
        return false;
    g_data.m_probe.m_param = m_param;
    wxDialog::TransferDataFromWindow();
    return true;
}

IMPLEMENT_CLASS(szViszioAddParam, wxDialog)
BEGIN_EVENT_TABLE(szViszioAddParam, wxDialog)
    EVT_BUTTON(ID_B_PARAM, szViszioAddParam::OnSelectParam)
END_EVENT_TABLE()
