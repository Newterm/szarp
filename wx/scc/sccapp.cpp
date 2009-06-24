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
 * scc - Szarp Control Center
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <wx/image.h>
#include <wx/cmdline.h>
#include <wx/wxprec.h>

#ifndef MINGW32
#include <unistd.h>
#include <sys/types.h>
#endif

#include "version.h"
#include "sccapp.h"
#include "cconv.h"
#include "sccframe.h"
#include "sccmenu.h"
#include "libpar.h"
#include "execute.h"
#include "ipc.h"

bool SCCApp::OnInit()
{
	scc_ipc_messages::init_ipc();

	if (szApp::OnInit() == false)
		return false;

	this->SetProgName(_("SCC"));

	if (reload_menu) {
		SCCClient* client = new SCCClient();
		SCCConnection* connection = client->Connect(scc_ipc_messages::scc_service);
		if (!connection) {
			printf("Cannot conncect to SCC\n");
		} else {
			wxChar* resp = connection->SendReloadMenuMsg();
			if (resp)
				printf("SCC said: %s\n", SC::S2A(wxString(resp)).c_str());
			else
				printf("No answer from SCC\n");
			free(resp);
		}
#if __WXGTK__
		exit(0);
#endif
		return false;
	} 

	const char *prefix;
	const char *suffix;
	char *buffer;
	int animate = 1;
	SCCMenu* smenu;
	app_instance = NULL;

#ifndef MINGW32
	app_instance2 = NULL;
#endif
	taskbar_item = NULL;

#ifndef __WXMSW__
	locale.Init();
#else //__WXMSW__
	locale.Init(wxLANGUAGE_POLISH);
#endif //__WXMSW__

	locale.AddCatalogLookupPathPrefix(GetSzarpDir() + _T("resources/locales"));

	locale.AddCatalog(_T("scc"));
	locale.AddCatalog(_T("common"));
	locale.AddCatalog(_T("wx"));

	wxLog *logger=new wxLogStderr();
	wxLog::SetActiveTarget(logger);

	wxImage::AddHandler( new wxXPMHandler );
#if wxUSE_LIBPNG
	wxImage::AddHandler( new wxPNGHandler );
#endif

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
	app_instance = new wxSingleInstanceChecker(_T(".scc"));
#ifndef MINGW32	
	wxString display;
	if (wxGetEnv(_T("DISPLAY"),&display)) {
		wxString idString = wxString::Format(_T(".scc-%s-%s"), wxGetUserId().c_str(), display.c_str());
		app_instance2 = new wxSingleInstanceChecker(idString);
	}
#endif
	
	if (app_instance->IsAnotherRunning()) {
#ifndef MINGW32	
		if (app_instance2 && !app_instance2->IsAnotherRunning()) 
			wxMessageBox(_("SCC program is not accessible.\n"
				"Most probably, you are seeing this message "
				"because you have logged in more than once."), 
				_("SCC - program already run"), 
				wxICON_HAND);
#endif
		return FALSE;
	}

	// Read params from szarp.cfg.
        prefix = libpar_getpar("scc", "images_prefix", 0);
        suffix = libpar_getpar("scc", "images_suffix", 0);
        buffer = libpar_getpar("scc", "animate", 0);
	if (buffer && (strcasecmp(buffer, "no") == 0))
		animate = 0;
	if (buffer)
		free(buffer);

	if (prefix == NULL)
		prefix = strdup(SC::S2A(GetSzarpDir() + _T("resources/wx/anim/praterm")).c_str());
	if (suffix == NULL)
		suffix = ".png";

	smenu = CreateMainMenu();
	// Create main frame.
	taskbar_item = new SCCTaskBarItem(smenu, wxString(SC::A2S(prefix)), wxString(SC::A2S(suffix)));

	server = new SCCServer(this, scc_ipc_messages::scc_service);
	
	SetAppName(_("SCC"));
	
	return true;
}

void SCCApp::ReloadMenu() {
#ifndef MINGW32
        libpar_reinit();
#else
        libpar_reinit_with_filename(SC::S2A((GetSzarpDir() + _T("resources/szarp.cfg"))).c_str(), 0);
#endif

	if (taskbar_item == NULL)
		return;
	SCCMenu *menu = CreateMainMenu();
	taskbar_item->ReplaceMenu(menu);
}

SCCMenu* SCCApp::CreateMainMenu() {
	char *buffer;
	SCCMenu* smenu;
        buffer = libpar_getpar("scc", "menu", 0);
	if (buffer == NULL) {
        	wxString s;
#ifndef MINGW32
		s = wxString(SC::A2S(libpar_getpar("scc", "prefix", 1)));
		s = s.AfterLast(_T('/'));
		
		s = _T("CONFIG(\"") + s;
		s += _T("\"), SEPARATOR, ");
#endif
		s += _T("DOC, \
			ICON(\"") + GetSzarpDir() + _T("resources/wx/icons/szarp16.xpm\")");
		smenu = SCCMenu::ParseMenu(s);
	} else {
		smenu = SCCMenu::ParseMenu(wxString(SC::A2S(buffer)));
		free(buffer);
	}

#ifndef MINGW32
	buffer = libpar_getpar("scc", "update_command", 0);
	if (buffer != NULL) {
		wxString command(wxString(SC::A2S(buffer)));
		free(buffer);

		if (wxFile::Exists(command)) {
			wxString sucommand;
			if (geteuid() != 0) {
        			buffer = libpar_getpar("scc", "su_command", 0);
				assert(buffer != NULL);
				sucommand << wxString(SC::A2S(buffer)) << _T(" -- ");
				free(buffer);
			}

			smenu->AddSeparator();
			smenu->AddCommand(_("Update data"), 
				sucommand + _T("xterm -e ") + command, NULL);
		} 
	}

#endif

	return smenu;

}

int SCCApp::OnExit()
{
	libpar_done();
	delete app_instance;
#ifndef MINGW32
	delete app_instance2;
#endif
	return 0;
}

void SCCApp::OnInitCmdLine(wxCmdLineParser &parser) {
	szApp::OnInitCmdLine(parser);
	parser.SetLogo(_T("SZARP Control Center. Notice that only one copy of program with specified\nparameters can be run."));
	parser.AddSwitch(_T("D<name>=<str>"), wxEmptyString,
		_T("libparnt value initialization"));
	parser.AddSwitch(_T("reload_menu"), wxEmptyString,
		_T("Tell othet scc to reload menu"));
	parser.AddParam(_T("additional data to allow multiple runs of program"),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL |
		wxCMD_LINE_PARAM_MULTIPLE);

}

bool SCCApp::OnCmdLineError(wxCmdLineParser &parser) {
	return true;
}

bool SCCApp::OnCmdLineHelp(wxCmdLineParser &parser) {
	parser.Usage();
	return false;
}

bool SCCApp::OnCmdLineParsed(wxCmdLineParser &parser) {
	if (parser.Found(_T("reload_menu"))) {
		reload_menu = true;
	} else
		reload_menu = false;

	return true;

}

