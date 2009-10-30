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
 * SZAU - Szarp Automatic Updater
 * SZARP

 * S³awomir Chy³ek schylek@praterm.com.pl
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <wx/image.h>
#include <wx/cmdline.h>
#include <wx/wxprec.h>
#include <wx/process.h>

#include <libxml/tree.h>

#ifndef MINGW32
#include <unistd.h>
#include <sys/types.h>
#endif

#include "version.h"
#include "szauapp.h"
#include "cconv.h"
#include "libpar.h"
#include "execute.h"
#include "szframe.h"

#ifndef MINGW32
#include "../../resources/wx/icons/szau_16.xpm"
#endif

IMPLEMENT_APP(SzauApp)

bool SzauApp::OnInit() {

	szApp::OnInit();
	this->SetProgName(_("SZAU - Szarp Automatic Updater"));

	char *rss_url;

	app_instance = NULL;
#ifndef MINGW32
	app_instance2 = NULL;
#endif

#ifndef __WXMSW__
	locale.Init();
#else //__WXMSW__
	locale.Init(wxLANGUAGE_POLISH);
#endif //__WXMSW_

	xmlInitParser();

	locale.AddCatalogLookupPathPrefix(GetSzarpDir() + _T("resources/locales"));

	locale.AddCatalog(_T("szau"));
	locale.AddCatalog(_T("common"));
	locale.AddCatalog(_T("wx"));

	wxLog *logger=new wxLogStderr();
	wxLog::SetActiveTarget(logger);

	wxImage::AddHandler(new wxXPMHandler);
#if wxUSE_LIBPNG
	wxImage::AddHandler(new wxPNGHandler);
#endif

	szFrame::setDefaultIcon(wxICON(szau_16));

#ifdef wxUSE_UNICODE
	libpar_read_cmdline_w(&argc, (wchar_t**) argv);
#else
	libpar_read_cmdline(&argc, argv);
#endif

#ifndef MINGW32
	libpar_init();
#else
	libpar_init_with_filename(SC::S2A((GetSzarpDir() + _T("resources/szarp.cfg"))).c_str(), 0);
#endif

	app_instance = new wxSingleInstanceChecker(_T(".szau"));
#ifndef MINGW32	
	wxString display;
	if (wxGetEnv(_T("DISPLAY"),&display)) {
		wxString idString = wxString::Format(_T(".szau-%s-%s"), wxGetUserId().c_str(), display.c_str());
		app_instance2 = new wxSingleInstanceChecker(idString);
	}
#endif

	if (app_instance->IsAnotherRunning()) {
#ifndef MINGW32	
		if (app_instance2 && !app_instance2->IsAnotherRunning())
			wxMessageBox(_("SZAU program is not accessible.\n"
					"Most probably, you are seeing this message "
					"because you have logged in more than once."),
			_("SZAU - program already run"),
			wxICON_HAND);
#endif
		return FALSE;
	}

	// Read params from szarp.cfg.
	rss_url = libpar_getpar("szau", "rss_url", 0);
	if (!rss_url)
		rss_url = strdup("http://sourceforge.net/api/file/index/project-id/210338/rss?path=/Windows");

	SetAppName(_("SZAU"));

	m_downloader = new Downloader(rss_url);

	Updater * updater = new Updater(m_downloader);

	fs::wpath current_installer = getInstallerLocalPath() / updater->VersionToInstallerName(SC::A2S(SZARP_VERSION)).c_str();
	try {
		fs::remove(current_installer);
	} catch (fs::filesystem_error err) { }
	
	UpdaterStatus status = NEWEST_VERSION;
	
	if (updater->InstallFileReadyOnStartup()) {
		int answer = wxMessageBox(_("New SZARP version is ready to install. Start installation?"), _("SZARP"), wxYES_NO, NULL);
		if (answer == wxYES)
			updater->Install();
		status = READY_TO_INSTALL;
	}
	
	updater->Start();

	m_taskbar = new SzauTaskBarItem(updater, status);

	return TRUE;
}

fs::wpath SzauApp::getInstallerLocalPath() {
	fs::wpath ret(std::wstring(wxGetHomeDir().wc_str()));
	ret /= L".szarp";
	if (!fs::exists(ret))
		create_directory(ret);

	return ret;
}

int SzauApp::OnExit() {
	delete m_downloader;
	libpar_done();
	delete app_instance;
#ifndef MINGW32
	delete app_instance2;
#endif
	return 0;
}
