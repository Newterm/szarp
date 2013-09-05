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
#include "koperapp.h"

#include "cconv.h"

bool KoperApp::ParseCommandLine() {
	wxCmdLineParser parser;
	parser.AddOption(_("d"), _("debug"), _("debug level (0-10)"), wxCMD_LINE_VAL_NUMBER);
	parser.AddSwitch(_("h"), _("help"), _("print usage info"));

	parser.SetCmdLine(argc, argv);

	if (parser.Parse(false) || parser.Found(_("h"))) {
		parser.Usage();
		return false;
	}

	long loglevel;
	if (parser.Found(_("d"), &loglevel)) {
		if (loglevel < 0 || loglevel > 10) {
			parser.Usage();
			return false;
		}
		sz_loginit(loglevel, NULL);
	}
	return true;
}

bool KoperApp::OnInit() {

	szApp<>::OnInit();

#ifndef __WXMSW__
	m_locale.Init();
#else //__WXMSW__
	m_locale.Init(wxLANGUAGE_POLISH);
#endif //__WXMSW__

	libpar_read_cmdline_w(&argc, argv);

#ifndef MINGW32
	libpar_init();
#else
	libpar_init_with_filename(SC::S2A((GetSzarpDir() + _T("resources/szarp.cfg"))).c_str(), 0);
#endif

	wxLog *logger=new wxLogStderr();
	wxLog::SetActiveTarget(logger);

#if wxUSE_LIBPNG
	wxImage::AddHandler(new wxPNGHandler());
#endif

	m_locale.AddCatalogLookupPathPrefix(GetSzarpDir() + _T("resources/locales"));

	m_locale.AddCatalog(_T("koper"));
	m_locale.AddCatalog(_T("common"));
	m_locale.AddCatalog(_T("wx"));

	if (!ParseCommandLine())
		return false;

	char *param, *date1, *date2, *prefix, *texfile, *backfile, *conefile, *fontfile;
	param = libpar_getpar("koper", "param", 1);
	date1 = libpar_getpar("koper", "1st_date_reset", 1);
	date2 = libpar_getpar("koper", "2nd_date_reset", 1);
	prefix = libpar_getpar("koper", "config_prefix", 1);
	texfile = libpar_getpar("koper", "texfile", 1);
	backfile = libpar_getpar("koper", "backfile", 1);
	fontfile = libpar_getpar("koper", "fontfile", 1);
	conefile = libpar_getpar("koper", "conefile", 1);


		
	m_app_inst = new wxSingleInstanceChecker(_T(".koper_lock_") + wxString(SC::A2S(prefix)));
	if (m_app_inst->IsAnotherRunning()) {
		delete m_app_inst;
		return false;
	}

	KoperValueFetcher* fetch = new KoperValueFetcher();
	if (!fetch->Init(SC::S2A(GetSzarpDataDir()), prefix, param, date1, date2)) 
		return false;
	

#if 0
	wxImage img;
	if (!img.LoadFile(szConv_sz2wx(texfile))) {
		wxLogError(_T("Not able to load texture file"));
		return FALSE;
	}

	wxImage bg;
	if (!bg.LoadFile(szConv_sz2wx(backfile))) {
		wxLogError(_T("Not able to load background texture file"));
		return FALSE;
	}
#endif

	KoperFrame *m_frame = new KoperFrame(fetch, texfile, backfile, conefile, fontfile, prefix);
	SetTopWindow(m_frame);
	m_frame->Show(TRUE);


	free(param);
	free(prefix);
	free(date1);
	free(date2);
	free(texfile);
	free(backfile);
	free(fontfile);

	SetAppName(_T("KOPER"));
	
	return TRUE;

}

int KoperApp::OnExit() {
	delete m_app_inst;
	return 0;
}

IMPLEMENT_APP(KoperApp)


