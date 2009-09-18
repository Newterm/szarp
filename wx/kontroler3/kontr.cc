/* $Id$
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

#include <wx/cmdline.h>
#include <wx/image.h>
#include <wx/config.h>

#include "libpar.h"

#include "kontr.h"
#include "kontroler.h"
#include "cconv.h"
#include "szapp.h"

/**
 * Main app class
 */
bool kontrApp::OnInit() {

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
    catalogs.Add(_T("kontroler3"));
    catalogs.Add(_T("common"));
    catalogs.Add(_T("wx"));
    this->InitializeLocale(catalogs, locale);

    if (!locale.IsOk()) {
      wxLogError(_T("Could not initialize locale."));
      return false;
    }

// To remove 
#if wxUSE_UNICODE
    libpar_read_cmdline_w(&argc, argv);
#else //wxUSE_UNICODE
    libpar_read_cmdline(&argc, argv);
#endif //wxUSE_UNICODE

    wxCmdLineParser parser;

    xmlInitParser();
    xmlSubstituteEntitiesDefault(1);
    szHTTPCurlClient::Init();

    wxString *geometry = new wxString;
    parser.SetLogo(_("Szarp Kontroler version 3.00 (in-progress)."));
    parser.AddOption(_T("geometry"), wxEmptyString, _("X windows geometry specification"), wxCMD_LINE_VAL_STRING);
    parser.AddSwitch(_T("D<name>=<str>"), wxEmptyString, _("libparnt value initialization"));
    parser.AddSwitch(_T("h"), _T("help"), _("print usage info"));
    parser.AddOption(_T("server"), wxEmptyString, _("server url"), wxCMD_LINE_VAL_STRING);
    parser.SetCmdLine(argc, argv);
    if (parser.Parse(false) || parser.Found(_T("h"))) {
      parser.Usage();
      return false;
    }

    if (!parser.Found(_T("geometry"), geometry)) {
      delete geometry;
      geometry = NULL;
    }

    wxString server;
    parser.Found(_T("server"), &server);
    wxLogError(_T("server: %s"), server.c_str());

#if wxUSE_LIBPNG
    wxImage::AddHandler( new wxPNGHandler() );
#endif //wxUSE_LIBPNG

    bool remote_mode = false;
    bool operator_mode = false;
    wxConfig::Get()->Read(_T("remote_mode"), &remote_mode);
    wxConfig::Get()->Read(_T("operator_mode"), &operator_mode);
    wxConfig::Get()->Flush();

    szKontroler *kontroler = new szKontroler(NULL, remote_mode, operator_mode, server);

    if (kontroler->loaded)  {
      return true;
    } else {
      delete geometry;
      delete kontroler;
      return false;
    }
}

IMPLEMENT_APP(kontrApp)
	
