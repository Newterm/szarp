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
 * raporter3 program
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <wx/cmdline.h>
#include <wx/image.h>
#include <wx/config.h>

#include "rap.h"
#include "raporter.h"
#include "cconv.h"
#include "httpcl.h"
#include "fonts.h"
#include "serverdlg.h"
#include "version.h"
#include "szframe.h"

#include "../../resources/wx/icons/rap16.xpm"

bool rapApp::OnCmdLineError(wxCmdLineParser &parser) {
	return true;
}

bool rapApp::OnCmdLineHelp(wxCmdLineParser &parser) {
	parser.Usage();
	return false;
}

bool rapApp::OnCmdLineParsed(wxCmdLineParser &parser) {
	if (parser.GetParamCount() > 0)
		m_server = parser.GetParam(0);
	if (parser.GetParamCount() > 1)
		m_title = parser.GetParam(1);
	return true;
}

void rapApp::OnInitCmdLine(wxCmdLineParser &parser) {
	szApp::OnInitCmdLine(parser);
	parser.SetLogo(_("Szarp Raporter v 3.1"));
	parser.AddSwitch(_T("h"), _("help"), _("show help"), wxCMD_LINE_OPTION_HELP);
	parser.AddParam(_T("server"), wxCMD_LINE_VAL_STRING,
			wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddParam(_T("title"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
}

bool rapApp::OnInit() 
{
	if (!szApp::OnInit())
		return false;
		
	srand(time(NULL));

	SetAppName(_T("raporter3"));

	this->SetProgName(_("Raporter 3"));
	
	wxLog *logger=new wxLogStderr();
	wxLog::SetActiveTarget(logger);

#if wxUSE_LIBPNG
	wxImage::AddHandler( new wxPNGHandler() );
#endif //wxUSE_LIBPNG

	//SET LOCALE
	wxArrayString catalogs;
	catalogs.Add(_T("raporter3"));
	catalogs.Add(_T("common"));
	catalogs.Add(_T("wx"));
	this->InitializeLocale(catalogs, locale);

	wxIcon icon(wxICON(rap16));
	if (icon.IsOk()) {
		szFrame::setDefaultIcon(icon);
	}

	xmlInitParser();
	szHTTPCurlClient::Init();

	if (m_server.IsEmpty()) {
		m_server = szServerDlg::GetServer(wxEmptyString, _T("Raporter"), false);
		if (m_server.IsEmpty())
			return false;
	}
	szRaporter* raporter = new szRaporter(NULL, m_server, m_title);

	SetTopWindow(raporter);
	SetExitOnFrameDelete(true);
	
	if (raporter->m_loaded)  {
		return true;
	} else {
		return false;
	}
}

int rapApp::OnExit()
{
	szHTTPCurlClient::Cleanup();
	szParList::Cleanup();
	xmlCleanupParser();
	szFontProvider::Cleanup();
	wxConfigBase* conf = wxConfig::Get();
	if (conf) delete conf;
	return 0;
}

IMPLEMENT_APP(rapApp)

