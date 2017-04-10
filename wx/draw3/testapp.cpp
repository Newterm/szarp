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
 * draw3
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/image.h>
#include <wx/cmdline.h>
#include <wx/config.h>
#include <wx/tooltip.h>
#include <wx/tokenzr.h>
#include <wx/xrc/xmlres.h>
#include <signal.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp> 

#ifdef MINGW32
#include <winsock2.h>
#include <sys/types.h>
#include <shlwapi.h>
#endif

#include <wx/evtloop.h>

#include "ids.h"
#include "classes.h"
#include "version.h"

#include "drawobs.h"
#include "libpar.h"
#include "liblog.h"

#include "cconv.h"
#include "cfgmgr.h"

#include "szapp.h"
#include "singleinstance.h"
#include "szmutex.h"
#include "szbase/szbbase.h"
#include "database.h"
#include "dbmgr.h"
#include "drawurl.h"
#include "errfrm.h"
#include "splashscreen.h"
#include "szframe.h"
#include "remarks.h"
#include "vercheck.h"
#include "getprobersaddresses.h"

#include "testrunner.h"
#include "testapp.h"

extern void InitXmlResource();

TestApp::TestApp() {
}

#if 0
int TestApp::OnRun() {
	wxEventLoop* loop = new wxEventLoop();

	m_runner->Run();

	//ProcessPendingEvents();
	loop->Run();

	return 0; 
}
#endif

bool TestApp::OnInit() {
	/* Read params from szarp.cfg. */
#if wxUSE_UNICODE
	libpar_read_cmdline_w(&argc, argv);
#else
	libpar_read_cmdline(&argc, argv);
#endif

	SetProgName(_T("Test app"));

	m_db_queue = NULL;
	m_executor = NULL;
	m_remarks_handler = NULL;

#ifdef __WXGTK__
	libpar_init();
#endif

#ifdef MINGW32
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);
#endif

#if 0
	wxPendingEventsLocker = new wxCriticalSection;
#endif

	/* Set logging to stderr. */
	wxLog *logger = new wxLogStderr();
	wxLog::SetActiveTarget(logger);

	wxString _lang = wxConfig::Get()->Read(_T("LANGUAGE"), AUTO_LANGUAGE);
	if (_lang == AUTO_LANGUAGE)
		_lang = DEFAULT_LANGUAGE;

	IPKContainer::Init(GetSzarpDataDir().wc_str(), 
			GetSzarpDir().wc_str(), 
			_lang.wc_str());
	m_cfg_mgr = new ConfigManager(GetSzarpDataDir().c_str(), IPKContainer::GetObject());

	m_db_queue = new DatabaseQueryQueue();
	m_dbmgr = new DatabaseManager(m_db_queue, m_cfg_mgr);
	m_db_queue->SetDatabaseManager(m_dbmgr);
	//m_dbmgr->SetProbersAddresses(GetProbersAddresses());

	m_executor = new QueryExecutor(m_db_queue, m_dbmgr, new SzbaseBase(
						m_dbmgr,
						GetSzarpDataDir().wc_str(),
						NULL,
						wxConfig::Get()->Read(_T("SZBUFER_IN_MEMORY_CACHE"), 0L)));
		
	m_executor->Create();
	m_executor->SetPriority((WXTHREAD_MAX_PRIORITY + WXTHREAD_DEFAULT_PRIORITY) / 2);
	m_executor->Run();

	m_cfg_mgr->SetDatabaseManager(m_dbmgr);

#ifndef MINGW32
	libpar_done();
#endif

	m_remarks_handler = new RemarksHandler(m_cfg_mgr);
	m_remarks_handler->Start();

	SetAppName(_T("SZARPTESTAPP"));

	m_runner = new TestRunner(m_dbmgr, m_cfg_mgr);
	m_runner->Run();

	return TRUE;
}

void TestApp::OnInitCmdLine(wxCmdLineParser &parser) {

        parser.SetLogo(_("Test app version 1.00."));

	parser.AddSwitch(_T("a") , _T("activity"));

	parser.AddOption(_T("base"), wxEmptyString, 
		_("base name"), wxCMD_LINE_VAL_STRING);

	parser.AddParam(_T("url"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	parser.AddSwitch(_T("e"), wxEmptyString, 
		_("open url in existing window"));


	parser.AddSwitch(_T("v"), _T("verbose"), _("verbose logging"));

	parser.AddSwitch(_T("V"), _T("version"), 
		_("print version number and exit"));
	
#if 0
	parser.AddSwitch(_T("D<name>=<str>"), wxEmptyString,
		_("libparnt value initialization"));
#endif
	
	parser.AddOption(_T("d"), _T("debug"), _("debug level"), wxCMD_LINE_VAL_NUMBER);
	

}

bool TestApp::OnCmdLineError(wxCmdLineParser &parser) {
	return true;
}

bool TestApp::OnCmdLineHelp(wxCmdLineParser &parser) {
	parser.Usage();
	return false;
}

bool TestApp::OnCmdLineParsed(wxCmdLineParser &parser) {

	if (parser.Found(_T("v")))
    		wxLog::SetVerbose();

	long debug;
	if (parser.Found(_T("debug"), &debug))
		sz_loginit((int) debug, "draw3", SZ_LIBLOG_FACILITY_APP);
	else
		sz_loginit(2, "draw3", SZ_LIBLOG_FACILITY_APP);

	return true;
}

void TestApp::StopThreads() {
	if (m_executor && m_db_queue) {
		m_db_queue->Add(NULL);
		m_executor->Wait();
	}
	if (m_remarks_handler)
		m_remarks_handler->Stop();
	delete m_db_queue;
}

int TestApp::OnExit() {
	StopThreads();

	return 0;
}

TestApp::~TestApp() {
}


