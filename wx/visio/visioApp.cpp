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
/* $Id: visioApp.cpp 1 2009-11-17 21:44:30Z asmyk $
 *
 * visio program
 * SZARP
 *
 * asmyko@praterm.com.pl
 */

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include "szapp.h"
#include "visioApp.h"
#include "visioFetchFrame.h"
#include "libpar.h"

IMPLEMENT_APP(visioApp);

bool visioApp::OnInit()
{
    if (szApp::OnInit() == false)
	    return false;

// To remove 
#if wxUSE_UNICODE
	libpar_read_cmdline_w(&argc, argv);
#else //wxUSE_UNICODE
	libpar_read_cmdline(&argc, argv);
#endif //wxUSE_UNICODE


    this->SetProgName(_("visio"));

    wxLog *logger=new wxLogStderr();
    wxLog::SetActiveTarget(logger);

#ifndef __WXMSW__
    locale.Init();
#else //__WXMSW__
    locale.Init(wxLANGUAGE_POLISH);
#endif //__WXMSW__

#ifndef __WXMSW__
    locale.AddCatalogLookupPathPrefix(wxGetApp().GetSzarpDir() + _T("resources/locales"));
#else //__WXMSW__
    locale.AddCatalogLookupPathPrefix(wxGetApp().GetSzarpDir() + _T("."));
#endif //__WXMSW__

    wxArrayString catalogs;
    catalogs.Add(_T("visio"));
    catalogs.Add(_T("common"));
    catalogs.Add(_T("wx"));
    this->InitializeLocale(catalogs, locale);

    if (!locale.IsOk()) {
      wxLogError(_T("Could not initialize locale."));
      return false;
    }

    xmlInitParser();
    xmlSubstituteEntitiesDefault(1);
    szHTTPCurlClient::Init();
	
    FetchFrame* frame = new FetchFrame(0L, m_server);
    frame->Show();
    return true;
}
