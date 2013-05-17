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

bool kontrApp::OnCmdLineError(wxCmdLineParser &parser) {
	return true;
}

bool kontrApp::OnCmdLineHelp(wxCmdLineParser &parser) {
	parser.Usage();
	return false;
}


bool kontrApp::OnCmdLineParsed(wxCmdLineParser &parser) {
	if (parser.GetParamCount() > 0)
		server = parser.GetParam(0);
	wxLogInfo(_T("server: %s"), server.c_str());

	return true;
}

void kontrApp::OnInitCmdLine(wxCmdLineParser &parser) {
	szApp<>::OnInitCmdLine(parser);

	parser.SetLogo(_("Szarp Kontroler version 3.00 (in-progress)."));
	parser.AddParam(_T("server"), wxCMD_LINE_VAL_STRING,
			wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(_T("D<name>=<str>"), wxEmptyString, _("libparnt value initialization"));
	parser.AddSwitch(_T("h"), _("help"), _("show help"), wxCMD_LINE_OPTION_HELP);
}

bool kontrApp::OnInit() {

    if (szApp<>::OnInit() == false)
	    return false;

// To remove 
#if wxUSE_UNICODE
	libpar_read_cmdline_w(&argc, argv);
#else //wxUSE_UNICODE
	libpar_read_cmdline(&argc, argv);
#endif //wxUSE_UNICODE


    this->SetProgName(_("Kontroler"));

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

    xmlInitParser();
    xmlSubstituteEntitiesDefault(1);
    szHTTPCurlClient::Init();


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
      delete kontroler;
      return false;
    }
}

IMPLEMENT_APP(kontrApp)
	
