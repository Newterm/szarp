#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include "TaskbarExampleMain.h"
#include <wx/listimpl.cpp>
WX_DEFINE_LIST(szProbeList);
#include <szarp_config.h>

#include <wx/listctrl.h>
#include <wx/list.h>
#include <wx/valtext.h>
#include <wx/valgen.h>

#include "libpar.h"

#include "htmlview.h"
#include "htmlabout.h"
#include "authdiag.h"

#include "fetchparams.h"
#include "parlist.h"


#include <wx/listctrl.h>
#include <wx/statusbr.h>
#include <wx/list.h>
#include <wx/colour.h>
#include <wx/sound.h>
#include <szarp_config.h>
#include <vector>

#include "fetchparams.h"
#include "serverdlg.h"

#include "cconv.h"
#include "authdiag.h"

szParamFetcher *TaskbarExampleFrame::m_pfetcher = NULL;
TSzarpConfig *TaskbarExampleFrame::ipk = NULL;
szProbeList TaskbarExampleFrame::m_probes;

enum wxbuildinfoformat
{
    short_f, long_f
};

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__WXMAC__)
        wxbuild << _T("-Mac");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}


TaskbarExampleFrame::TaskbarExampleFrame(wxFrame *frame)
        : TransparentFrame(frame)
{
#if wxUSE_STATUSBAR
//   statusBar->SetStatusText(_("Hello Code::Blocks user!"), 0);
    //  statusBar->SetStatusText(wxbuildinfo(short_f), 1);
#endif
    m_http = new szHTTPCurlClient();
    bool ask_for_server=false;
    //m_server = _("83.16.25.83:8085");
    m_server = _("localhost:8087");
    while (ipk == NULL)
    {
        m_server = szServerDlg::GetServer(m_server, _T("Visio"), ask_for_server);
        if (m_server.IsEmpty() or m_server.IsSameAs(_T("Cancel")))
        {
            exit(0);
        }
        ipk = szServerDlg::GetIPK(m_server, m_http);
        if (ipk == NULL)
            ask_for_server = true;
        m_apd = new szVisioAddParam(this->ipk, this, wxID_ANY, _("Visio->AddParam"));
        szProbe *probe = new szProbe();
        m_apd->g_data.m_probe.Set(*probe);

        if ( m_apd->ShowModal() != wxID_OK )
        {
            exit(0);
        }
        probe->Set(m_apd->g_data.m_probe);

        if (m_pfetcher==NULL)
        {
            m_pfetcher = new szParamFetcher(m_server, this, m_http);
            m_pfetcher->GetParams().RegisterIPK(ipk);
            m_pfetcher->SetPeriod(10);
        }

        m_probes.Append(probe);
//	m_parameter_name->SetText(_((probe->m_param->GetShortName()):_("(none)")).c_str()));
//	m_parameter_name->SetText( _(probe->m_param->GetShortName().c_str()) );
//  m_parameter_name->SetText( wxString((probe->m_param != NULL)?(probe->m_param->GetShortName()):L"(none)").c_str() );
        m_parameter_name->SetText( wxString((probe->m_param != NULL)?(probe->m_param->GetName()):L"(none)").c_str() );
        m_parameter_name->SetFont(*m_font, *m_font_color);
        m_pfetcher->SetSource(m_probes, ipk);
        //m_pfetcher->Run();
        /*
        	wxLogMessage(_T("par_add: ok (\"%s\", %s, %d, %d, %f, %f, %d, %d, %d, %d, \"%s\")"),
                probe->m_parname.c_str(),
                wxString((probe->m_param != NULL)?(probe->m_param->GetShortName()):L"(none)").c_str(),
                probe->m_formula, probe->m_value_check,
                probe->m_value_min, probe->m_value_max,
                probe->m_data_period, probe->m_precision,
                probe->m_alarm_type, probe->m_alarm_group,
                probe->m_alt_name.c_str());
        */



    }

}

TaskbarExampleFrame::~TaskbarExampleFrame()
{
}

void TaskbarExampleFrame::OnClose(wxCloseEvent &event)
{
    Destroy();
}

void TaskbarExampleFrame::OnQuit(wxCommandEvent &event)
{
    Destroy();
}

void TaskbarExampleFrame::OnAbout(wxCommandEvent &event)
{
    wxString msg = wxbuildinfo(long_f);
    wxMessageBox(msg, _("Welcome to..."));
}
