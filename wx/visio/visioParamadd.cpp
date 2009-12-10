#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/valtext.h>
#include <wx/valgen.h>

#include "visioParamadd.h"
#include "cconv.h"


szVisioAddParam::szVisioAddParam(TSzarpConfig *_ipk,
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
    ps = new szParSelect(this->ipk, this, wxID_ANY, _("Visio->Add->Param"),
                         TRUE, TRUE, false, false,true);
//    pc = new szParCalc(this->ipk,
    //    this, wxID_ANY, _("Visio->Add->Formula"));

    top_sizer->Fit(this);
}

szVisioAddParam::~szVisioAddParam()
{
    delete ps;
}

void szVisioAddParam::OnSelectParam(wxCommandEvent &ev)
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



bool szVisioAddParam::TransferDataToWindow()
{

    m_param = g_data.m_probe.m_param;
    wxDialog::TransferDataToWindow();
    return true;
}

bool szVisioAddParam::TransferDataFromWindow()
{
    if ( m_param == NULL )
        return false;

    g_data.m_probe.m_param = m_param;
#if 0
    g_data.m_probe.m_precision = (wxStaticCast(FindWindowById(ID_SC_PREC),
                                  wxSpinCtrl)->GetValue());
#endif
    wxDialog::TransferDataFromWindow();

//  g_data.m_probe.m_alarm_type = (wxDynamicCast(FindWindowById(ID_CH_ALARM), wxChoice)->GetSelection()) + 1;

//  m_value_min.ToDouble(&g_data.m_probe.m_value_min);
//  m_value_max.ToDouble(&g_data.m_probe.m_value_max);

    return true;
}

IMPLEMENT_CLASS(szVisioAddParam, wxDialog)
BEGIN_EVENT_TABLE(szVisioAddParam, wxDialog)
    EVT_BUTTON(ID_B_PARAM, szVisioAddParam::OnSelectParam)
END_EVENT_TABLE()
