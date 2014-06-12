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
/*
 * ekstraktor3 program
 * SZARP
 
 * vooyeck@praterm.com.pl
 
 * $Id$
 */

// For compilers that support precompilation, includes "wx/wx.h".
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
#include <wx/filename.h>
#include "libpar.h"

// the application widget 
#include "EkstraktorWidget.h"
#include "geometry.h"
#include "cconv.h"
#include "szapp.h"
#include "getprobersaddresses.h"
#include "szframe.h"

#include "ekstraktor3.h"

#include "../../resources/wx/icons/extr64.xpm"

bool EkstrApp::OnInit()
{
	//  READ PARAMS FROM CMD LINE
#if wxUSE_UNICODE
	libpar_read_cmdline_w(&argc, argv);
#else
	libpar_read_cmdline(&argc, argv);
#endif
	if (!szApp<>::OnInit())
		return false;

#if BOOST_FILESYSTEM_VERSION == 3
	boost::filesystem::wpath::imbue(std::locale("C")); 	
#else
	boost::filesystem::wpath_traits::imbue(std::locale("C")); 	
#endif

	SetProgName(_("Ekstraktor 3"));

	// SET LOCALE
	wxArrayString catalogs;
	catalogs.Add(_T("ekstraktor3"));
	catalogs.Add(_T("common"));
	catalogs.Add(_T("wx"));
	InitializeLocale(catalogs, locale);

	// GET LIBPAR STUFF
#ifndef MINGW32
	libpar_init();
#else
	wxString config_str =
	    GetSzarpDir() + wxFileName::GetPathSeparator() + _T("resources") +
	    wxFileName::GetPathSeparator() + _T("szarp.cfg");
	libpar_init_with_filename(SC::S2A(config_str).c_str(), 1);
#endif
	std::wstring ipk_prefix;
	char *_ipk_prefix = libpar_getpar("", "config_prefix", 0);
	if (_ipk_prefix) {
		ipk_prefix = SC::L2S(_ipk_prefix);
		free(_ipk_prefix);
	}
	std::map<wxString, std::pair<wxString, wxString> > m_probers_addresses;
#ifndef MINGW32
	m_probers_addresses = get_probers_addresses();
#endif
	szFrame::setDefaultIcon(wxICON(extr64));

	if (!base.IsEmpty()) {
		ipk_prefix = base.c_str();
	} else {
		if (ipk_prefix.empty()) {
			wxArrayString hidden_databases;
			wxString tmp;
			wxConfigBase *config = wxConfigBase::Get(true);
			if (config->Read(_T("HIDDEN_DATABASES"), &tmp)) {
				wxStringTokenizer tkz(tmp, _T(","), wxTOKEN_STRTOK);
				while (tkz.HasMoreTokens()) {
					wxString token = tkz.GetNextToken();
					token.Trim();
					if (!token.IsEmpty())
						hidden_databases.Add(token);
				}
			}
			if (ConfigDialog::SelectDatabase(base, &hidden_databases)) {
				ipk_prefix = base;
			} else
				return false;
		}
	}

	libpar_done();

#ifdef MINGW32
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);
#endif

	xmlInitParser();
	LIBXML_TEST_VERSION xmlSubstituteEntitiesDefault(1);

	// .. AND START THE MAIN WIDGET
	std::pair<wxString, wxString> prober_address;
	if (m_probers_addresses.find(ipk_prefix) != m_probers_addresses.end())
		prober_address = m_probers_addresses[ipk_prefix];
	EkstraktorWidget *ew = new EkstraktorWidget(ipk_prefix, geometry.IsEmpty() ? NULL : &geometry, prober_address);

	if (ew->IsConfigLoaded() == false) {
		delete ew;
		exit(1);
	}
	return true;

} // onInit()

bool EkstrApp::OnCmdLineError(wxCmdLineParser &parser) {
	return true;
}

bool EkstrApp::OnCmdLineHelp(wxCmdLineParser &parser) {
	parser.Usage();
	return false;
}

bool EkstrApp::OnCmdLineParsed(wxCmdLineParser &parser) {
	if (!parser.Found(_T("geometry"), &geometry)) {
		geometry = wxString();
	}
	if (!parser.Found(_T("base"), &base)) {
		base = wxString();
	}
	return true;
}

void EkstrApp::OnInitCmdLine(wxCmdLineParser &parser) {
	szApp<>::OnInitCmdLine(parser);
	parser.SetLogo(_("Szarp Extractor version 3.00"));
	parser.AddOption(_T("geometry"), wxEmptyString,
			 _("X windows geometry specification"),
			 wxCMD_LINE_VAL_STRING);
	parser.AddOption(_T("base"), wxEmptyString, _("base name"),
			 wxCMD_LINE_VAL_STRING);
}

IMPLEMENT_APP(EkstrApp)
