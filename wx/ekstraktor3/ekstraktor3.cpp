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
#include "szframe.h"

#include "ekstraktor3.h"

#include "../../resources/wx/icons/extr64.xpm"

bool EkstrApp::OnInit()
{
	szApp::OnInit();
	this->SetProgName(_("Ekstraktor 3"));

	// SET LOCALE
	wxArrayString catalogs;
	catalogs.Add(_T("ekstraktor3"));
	catalogs.Add(_T("common"));
	catalogs.Add(_T("wx"));
	this->InitializeLocale(catalogs, locale);

	//  READ PARAMS FROM CMD LINE
#if wxUSE_UNICODE
	libpar_read_cmdline_w(&argc, argv);
#else
	libpar_read_cmdline(&argc, argv);
#endif

	// PARSE CMD LINE
	wxCmdLineParser parser;
	wxString *geometry = new wxString;

	// SET CMD LINE INFO
	parser.SetLogo(_("Szarp Extractor version 3.00"));
	parser.AddOption(_T("geometry"), wxEmptyString,
			 _("X windows geometry specification"),
			 wxCMD_LINE_VAL_STRING);
	parser.AddOption(_T("base"), wxEmptyString, _("base name"),
			 wxCMD_LINE_VAL_STRING);
	parser.AddSwitch(_T("h"), _T("help"), _("print usage info"));
	parser.SetCmdLine(argc, argv);
	if (parser.Parse(false) || parser.Found(_T("h"))) {
		parser.Usage();
		exit(1);
	}
	// READ PROGRAM GEOMETRY
	if (!parser.Found(_T("geometry"), geometry)) {
		delete geometry;
		geometry = NULL;
	}
	// GET LIBPAR STUFF
#ifndef MINGW32
	libpar_init();
#else
	wxString config_str =
	    GetSzarpDir() + wxFileName::GetPathSeparator() + _T("resources") +
	    wxFileName::GetPathSeparator() + _T("szarp.cfg");
	libpar_init_with_filename(SC::S2A(config_str).c_str(), 1);
#endif
	std::wstring xml_prefix;
	char *_xml_prefix = libpar_getpar("", "config_prefix", 0);
	if (_xml_prefix) {
		xml_prefix = SC::A2S(_xml_prefix);
		free(_xml_prefix);
	}

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

	szFrame::setDefaultIcon(wxICON(extr64));

	wxString base;
	if (parser.Found(_T("base"), &base)) {
		xml_prefix = base;
	} else {
		if (ConfigDialog::SelectDatabase(base, &hidden_databases)) {
			xml_prefix = base;
		} else
			exit(1);
	}

	libpar_done();

	xmlInitParser();
	LIBXML_TEST_VERSION xmlSubstituteEntitiesDefault(1);

	// .. AND START THE MAIN WIDGET
	EkstraktorWidget *ew = new EkstraktorWidget(xml_prefix, geometry);

	if (ew->IsConfigLoaded() == false) {
		delete ew;
		//delete logger;
		delete geometry;

		exit(1);
	}
	return true;

} // onInit()

IMPLEMENT_APP(EkstrApp)
