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
/* $Id: viszioFetchFrame.cpp 1 2009-11-17 21:44:30Z asmyk $
 *
 * viszio program
 * SZARP
 *
 * asmyk@praterm.com.pl
 */
 
#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include "viszioFetchFrame.h"
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

#include "fetchparams.h"
#include "parlist.h"


#include <wx/listctrl.h>
#include <wx/statusbr.h>
#include <wx/list.h>
#include <wx/colour.h>
#include <wx/sound.h>
#include <szarp_config.h>
#include <vector>
#include "serverdlg.h"
#include "cconv.h"

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

BEGIN_EVENT_TABLE( FetchFrame, TransparentFrame )
    EVT_SZ_FETCH(FetchFrame::OnFetch)
END_EVENT_TABLE()

FetchFrame::FetchFrame(wxFrame *frame, wxString server, szHTTPCurlClient *http, wxString paramName)
        : TransparentFrame(frame, true, paramName)
{
	m_typeOfFrame = FETCH_TRANSPARENT_FRAME;
	m_server = server;
	m_http = http;
	if (m_pfetcher == NULL)
    {
		m_pfetcher = new szParamFetcher(m_server, this, m_http);
        m_pfetcher->GetParams().RegisterIPK(TransparentFrame::ipk);
        m_pfetcher->SetPeriod(10);
	}	
    m_pfetcher->SetSource(m_probes, TransparentFrame::ipk);    
    if(m_pfetcher->IsAlive())
		m_pfetcher->Resume();
    else
		m_pfetcher->Run();
}


FetchFrame::~FetchFrame()
{
}

void FetchFrame::OnClose(wxCloseEvent &event)
{
    Destroy();
}

void FetchFrame::OnQuit(wxCommandEvent &event)
{
    Destroy();
}

void FetchFrame::OnAbout(wxCommandEvent &event)
{
    wxString msg = wxbuildinfo(long_f);
    wxMessageBox(msg, _("Welcome to..."));
}

void FetchFrame::OnFetch(wxCommandEvent& event)
{
    int w = 0, h = 0;	
    wxSize size = GetClientSize();
    w = size.GetWidth();
    h = size.GetHeight();	
	m_pfetcher->Lock();		
	if (m_pfetcher->IsValid() == true)
	{		
		for (size_t i = 0; i < m_pfetcher->GetParams().Count(); i++) {
			wxString name = m_pfetcher->GetParams().GetName(i);
			wxString val = m_pfetcher->GetParams().GetExtraProp(i, SZ_REPORTS_NS_URI, _T("value"));
			wxString unit = m_pfetcher->GetParams().GetExtraProp(i, SZ_REPORTS_NS_URI, _T("unit"));
					
			for(int j=0; j<TransparentFrame::max_number_of_frames; j++)
			{
				if(all_frames[j] != NULL && all_frames[j]->GetParameterName().Cmp(name) == 0)
				{
					all_frames[j]->SetParameterValue(val + _(" ") + unit);
					all_frames[j]->RefreshTransparentFrame();
					break;
				}
			}
		}
	}
	m_pfetcher->Unlock();
}
