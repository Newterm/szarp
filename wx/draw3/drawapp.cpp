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

#ifndef NO_GSTREAMER
#include <gst/gst.h>
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

#ifdef HAVE_GLCANVAS
#include <wx/glcanvas.h>
#endif

#include "ids.h"
#include "classes.h"
#include "version.h"

#include "drawobs.h"
#include "drawapp.h"
#include "drawipc.h"
#include "libpar.h"
#include "liblog.h"

#include "cconv.h"
#include "geometry.h"
#include "cfgmgr.h"
#include "frmmgr.h"

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

#include "../../resources/wx/icons/draw64.xpm"

extern void InitXmlResource();

class ConfigurationFileChangeHandler{
	public:
		static DatabaseManager *database_manager;
		static void handle(std::wstring file, std::wstring prefix);
};

void
ConfigurationFileChangeHandler::handle(std::wstring file, std::wstring prefix) {
	ConfigurationChangedEvent e(prefix);
	wxPostEvent(database_manager, e);
};

DatabaseManager* ConfigurationFileChangeHandler::database_manager(NULL);

void INThandler(int sig)
{
	wxGetApp().OnExit();
	exit(0);
}

namespace {
int GL_ATTRIBUTES[] = { 
	WX_GL_RGBA,
	WX_GL_DOUBLEBUFFER,
	WX_GL_DEPTH_SIZE, 16,
#if 0
	WX_GL_SAMPLE_BUFFERS, GL_TRUE,
	WX_GL_SAMPLES, 4,
#endif
	0 };
}

DrawApp::DrawApp() : DrawGLApp(), m_base_type(SZBASE_BASE), m_iks_port(_T("9002")) {
	m_gl_context = NULL;
}

wxGLContext* DrawApp::GetGLContext() {
	if (m_gl_context)
		return m_gl_context;

	if (!m_gl_works)
		return NULL;

#ifdef HAVE_GLCANVAS
	wxGLCanvas *tcanvas  = new wxGLCanvas(GetTopWindow(), wxID_ANY, GLContextAttribs());
	m_gl_context = new wxGLContext(tcanvas);
	tcanvas->Destroy();
#endif
	return m_gl_context;
}

void DrawApp::InitGL() { 

	bool gl_init_failed = false;
	wxConfig::Get()->Read(_T("GL_INIT_FAILED"), &gl_init_failed);
	if (!gl_init_failed) {
		if (InitGLVisual(GL_ATTRIBUTES)) {
			m_gl_context_attribs = GL_ATTRIBUTES;
			m_gl_works = true;
		} else if (InitGLVisual(NULL)) {
			m_gl_context_attribs = NULL;
			m_gl_works = true;
		} else {
			m_gl_works = false;
		}

		wxConfig::Get()->Write(_T("GL_INIT_FAILED"), false);
		wxConfig::Get()->Flush();

	} else {
		m_gl_works = false;
	}
}


bool DrawApp::OnInit() {


	/* Read params from szarp.cfg. */
#if wxUSE_UNICODE
	libpar_read_cmdline_w(&argc, argv);
#else
	libpar_read_cmdline(&argc, argv);
#endif

#if BOOST_FILESYSTEM_VERSION == 3
	boost::filesystem::wpath::imbue(std::locale("C")); 	
#else
	boost::filesystem::wpath_traits::imbue(std::locale("C")); 	
#endif

	if (!DrawGLApp::OnInit())
		return false;


	SetProgName(_T("Draw 3"));
	//signal(SIGINT, INThandler);

	if (m_just_print_version) {
		std::cout << SZARP_VERSION << std::endl;
		return false;
	}
	m_server = NULL;
	m_db_queue = NULL;
	m_executor = NULL;
	m_remarks_handler = NULL;

#ifdef __WXGTK__
	libpar_init();

	char *base = NULL;
	if (m_base == wxEmptyString) {
		base = libpar_getpar("", "config_prefix", 1);
		m_base = SC::L2S(base);
		free(base);
	}

	{

		if (m_base_type != IKS_BASE) {
			char *iks_server = libpar_getpar("draw3", "iks_server", 0);
			if (iks_server) {
				m_iks_server = SC::L2S(iks_server);
				m_base_type = IKS_BASE;
				free(iks_server);
			}

			char *iks_port = libpar_getpar("draw3", "iks_server_port", 0);
			if (iks_port) {
				m_iks_port = SC::L2S(iks_port);
				free(iks_port);
			}
		}
	}

	m_probers_from_szarp_cfg = get_probers_addresses();
#endif

#ifdef MINGW32
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);
#endif
	/* Set logging to stderr. */
	wxLog *logger = new wxLogStderr();
	//wxLog *logger = new wxLogGui();
	wxLog::SetActiveTarget(logger);


	m_instance = new szSingleInstanceChecker(_T(".szarp_m_instance_lock"), wxEmptyString, 
			_T("draw3"));
	if (m_instance->IsAnotherRunning()) {
		if (!m_url.IsEmpty()) {
			if (m_url_open_in_existing)
				SendToRunningInstance(_T("START_URL"), m_url.c_str());
			else
				SendToRunningInstance(_T("START_URL_EXISTING"), m_url.c_str());
		} else if (!m_base.IsEmpty()) {		
			SendToRunningInstance(_T("START_BASE"), m_base.c_str());
		} else {
			wxLogError(_T("base not given"));
			return false;
		}
		return false;
	}
        

	InitGL();

	SplashScreen *splash = NULL;
	wxBitmap bitmap;

	// this loads draw64_xpm under Linux, or draw64 icon resource under Windows
	szFrame::setDefaultIcon(wxICON(draw64));

	wxString splashimage = GetSzarpDir();

#ifndef MINGW32
	splashimage += _T("resources/wx/images/szarp-logo.png");
#else
	splashimage += _T("resources\\wx\\images\\szarp-logo.png");
#endif

#if wxUSE_LIBPNG
	/* Activate PNG image handler. */
	wxImage::AddHandler( new wxPNGHandler );
#endif

	if (bitmap.LoadFile(splashimage, wxBITMAP_TYPE_PNG))
		splash = new SplashScreen(&bitmap);
	else
		splash = new SplashScreen();
	splash->Show(true);

	splash->PushStatusText(_("Setting locale..."));
	
	/* Set locale. */
	this->InitializeLocale(_T("draw3"), locale);

	splash->PushStatusText(_("Reading params from szarp.cfg..."));

	wxString _lang = wxConfig::Get()->Read(_T("LANGUAGE"), AUTO_LANGUAGE);
	if (_lang == AUTO_LANGUAGE)
		_lang = DEFAULT_LANGUAGE;

	splash->PushStatusText(_("Initializing IPKContainer..."));
	IPKContainer::Init(GetSzarpDataDir().c_str(), 
			GetSzarpDir().c_str(), 
			_lang.c_str());
	m_cfg_mgr = new ConfigManager(GetSzarpDataDir(), IPKContainer::GetObject());

	m_cfg_mgr->SetSplashScreen(splash);

	splash->PushStatusText(_("Initializing szbase..."));

	splash->PushStatusText(_("Initializing XML Resources..."));
    
	InitXmlResource();

	splash->PushStatusText(_("Starting database query mechanism..."));

	m_db_queue = new DatabaseQueryQueue();
	m_dbmgr = new DatabaseManager(m_db_queue, m_cfg_mgr);
	m_db_queue->SetDatabaseManager(m_dbmgr);
	m_dbmgr->SetProbersAddresses(GetProbersAddresses());

	Draw3Base* draw_base;
	switch (m_base_type) {
		case SZBASE_BASE:
			draw_base = new SzbaseBase(m_dbmgr, GetSzarpDataDir().c_str(),
				&ConfigurationFileChangeHandler::handle,
				wxConfig::Get()->Read(_T("SZBUFER_IN_MEMORY_CACHE"), 0L));
			break;
		case SZ4_BASE:
			draw_base = new Sz4Base(m_dbmgr, GetSzarpDataDir().c_str(),
				IPKContainer::GetObject());
			break;
		case IKS_BASE:
			draw_base = new Sz4ApiBase(m_dbmgr, m_iks_server.c_str(), m_iks_port.c_str(),
				IPKContainer::GetObject());
			break;
	}

	m_executor = new QueryExecutor(m_db_queue, m_dbmgr, draw_base);
	m_executor->Create();
	m_executor->SetPriority((WXTHREAD_MAX_PRIORITY + WXTHREAD_DEFAULT_PRIORITY) / 2);
	m_executor->Run();

	m_cfg_mgr->SetDatabaseManager(m_dbmgr);

	ConfigurationFileChangeHandler::database_manager = m_dbmgr;

	/* default config */
	wxString defid;

	DrawsSets *config = NULL;

	splash->PushStatusText(_("Loading configuration..."));

	/* init activity logger with base name as server name. This assumes that 
	 * server has written basename to /etc/hosts on linux machines and probably
	 * doesn't work on windows.
	 */
	UDPLogger::SetAppName( "draw3" );
	UDPLogger::SetAddress( (const char*)m_base.mb_str(wxConvUTF8) );
	UDPLogger::SetPort   ( "7777" );
	UDPLogger::LogEvent  ( "drawapp:start" );



	wxString prefix, window;
	PeriodType pt = PERIOD_T_YEAR;
	time_t time = -1;
	int selected_draw = 0;

	if (!m_url.IsEmpty()) {
		if (!decode_url(m_url, prefix, window, pt, time, selected_draw)) {
			wxLogError(_T("Invalid URL"));
			StopThreads();
			return FALSE;
		}
	} else if (!m_base.IsEmpty()) {
		if ((config = m_cfg_mgr->LoadConfig(m_base,std::wstring(),m_show_logparams)) == NULL) {
			wxLogError(_("Error occurred while loading default configuration. Check your szarp.cfg file."));
			StopThreads();
			return FALSE;
		}
		prefix = m_base;
	} else {
		wxLogError(_("Missing starting base specification."));
		StopThreads();
		return FALSE;
	}


#ifndef MINGW32
	libpar_done();
#endif

	m_help = new szHelpController;
#ifndef MINGW32
	m_help->AddBook(wxGetApp().GetSzarpDir() + L"/resources/documentation/new/draw3/html/draw3.hhp");
#else
	m_help->AddBook(wxGetApp().GetSzarpDir() + L"\\resources\\documentation\\new\\draw3\\html\\draw3.hhp");
#endif
	szHelpControllerHelpProvider* provider = new szHelpControllerHelpProvider;
	wxHelpProvider::Set(provider);
	provider->SetHelpController(m_help);

	splash->PushStatusText(_("Initizalizing remarks..."));
	
	m_remarks_handler = new RemarksHandler(m_cfg_mgr);
	m_remarks_handler->Start();
	
	splash->PushStatusText(_("Creating Frame Manager..."));

	VersionChecker* version_checker = new VersionChecker(argv[0]);
	version_checker->Start();

	FrameManager *fm = new FrameManager(m_dbmgr, m_cfg_mgr, m_remarks_handler);

	/*@michal  */
	if (!fm->CreateFrame(prefix, window, pt, time, wxSize(width, height), wxPoint(x, y), selected_draw, m_url.IsEmpty(), m_full_screen)) {
		StopThreads();
		wxLogError(_T("Unable to load base: %s"), prefix.c_str());
		return FALSE;
	}

	StartInstanceServer(fm, m_cfg_mgr);

	wxToolTip::SetDelay(1000);
    
	SetAppName(_T("SZARPDRAW3"));

	splash->Destroy();

	m_cfg_mgr->ResetSplashScreen();

#ifndef NO_GSTREAMER
	if (!gst_init_check(NULL, NULL, NULL)) 
		return 1;
#endif

	return TRUE;
}

void DrawApp::OnInitCmdLine(wxCmdLineParser &parser) {
	wxGLApp::OnInitCmdLine(parser);

        parser.SetLogo(_("Szarp Draw version 3.00."));

	parser.AddOption(_T("geometry"), wxEmptyString, 
		_("X windows geometry specification"), wxCMD_LINE_VAL_STRING);

	parser.AddSwitch(_T("a") , _T("activity"));

	parser.AddOption(_T("base"), wxEmptyString, 
		_("base name"), wxCMD_LINE_VAL_STRING);

	parser.AddParam(_T("url"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	parser.AddSwitch(_T("e"), wxEmptyString, 
		_("open url in existing window"));

	parser.AddSwitch(_T("f"), wxEmptyString, 
		_("open window in full screen mode"));

	parser.AddSwitch(_T("v"), _T("verbose"), _("verbose logging"));

	parser.AddSwitch(_T("4"), _T("sz4"), _("use sz4 base format"));

	parser.AddSwitch(_T("i"), _T("iks"), _("use iks server"));

	parser.AddSwitch(_T("V"), _T("version"), 
		_("print version number and exit"));
	
	parser.AddOption(wxEmptyString, _T("iks-server"), 
		_("IKS server address"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	parser.AddOption(wxEmptyString, _T("iks-server-port"),
		_("IKS server port"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
#if 0
	parser.AddSwitch(_T("D<name>=<str>"), wxEmptyString,
		_("libparnt value initialization"));
#endif
	
	parser.AddOption(_T("d"), _T("debug"), _("debug level"), wxCMD_LINE_VAL_NUMBER);
	

}

bool DrawApp::OnCmdLineError(wxCmdLineParser &parser) {
	return true;
}

bool DrawApp::OnCmdLineHelp(wxCmdLineParser &parser) {
	parser.Usage();
	return false;
}

bool DrawApp::OnCmdLineParsed(wxCmdLineParser &parser) {

	wxString geometry;

        /* Reads program geometry (X style) */

	x = y = width = height = -1;

        /* Read 'geometry' option. */
	if (parser.Found(_T("geometry"), &geometry)) 
		get_geometry(geometry, &x, &y, &width, &height);

	if (parser.Found(_T("v")))
    		wxLog::SetVerbose();

	m_full_screen = parser.Found(_T("f"));
	
	m_just_print_version = parser.Found(_("V"));

	parser.Found(_T("base"), &m_base);

	parser.Found(_T("url"), &m_url);

	m_show_logparams = parser.Found(_T("activity"));

	m_url_open_in_existing = parser.Found(_T("e"));

	if (m_base.IsEmpty())
		for (size_t i = 0; i < parser.GetParamCount(); ++i) {
			wxString arg = parser.GetParam(i);
			if (arg.StartsWith(_T("draw://"))) {
				m_url = arg;	
				break;
			}
		}

	long debug;
	if (parser.Found(_T("debug"), &debug))
		sz_loginit((int) debug, "draw3", SZ_LIBLOG_FACILITY_APP);
	else
		sz_loginit(2, "draw3", SZ_LIBLOG_FACILITY_APP);

	if (parser.Found(_T("4")))
		m_base_type = SZ4_BASE;

	if (parser.Found(_T("i"))) {
		m_base_type = IKS_BASE;

		if (!parser.Found(_T("iks-server"), &m_iks_server))
			m_iks_server = m_base;

		parser.Found(_T("iks-server-port"), &m_iks_port);
	}

	return true;
}

void DrawApp::StopThreads() {
	if (m_executor && m_db_queue) {
		m_db_queue->Add(NULL);
		m_executor->Wait();
	}
	if (m_remarks_handler)
		m_remarks_handler->Stop();
#if 0
	delete m_db_queue;
	delete m_remarks_handler;
#endif
}

int DrawApp::OnExit() {
	delete m_instance;
	delete m_server;

	StopThreads();

	m_cfg_mgr->SaveDefinedDrawsSets();

	wxConfig::Get()->Flush();

	ErrorFrame::DestroyWindow();

	return 0;
}

void DrawApp::StartInstanceServer(FrameManager *frmmgr, ConfigManager *cfgmgr) {
	m_server = new DrawServer(frmmgr, cfgmgr);

	wxString service = wxGetHomeDir() + _T("/.draw3_socket");

	if (m_server->Create(service) == false) {
		wxLogError(_T("Failed to start server"));
		delete m_server;
	}
}

void DrawApp::SendToRunningInstance(wxString topic, wxString msg) {
	wxString service = wxGetHomeDir() + _T("/.draw3_socket");
	DrawClient *client = new DrawClient(service);
	client->SendMsg(topic, msg);
	delete client;
}

void DrawApp::DisplayHelp() {
	m_help->DisplayContents();
}

std::map<wxString, std::pair<wxString, wxString> > DrawApp::GetProbersAddresses() {
	std::map<wxString, std::pair<wxString, wxString> > ret = m_probers_from_szarp_cfg;
	wxConfigBase* config = wxConfigBase::Get(true);
	wxString tmp;
	if (config->Read(_T("PROBE_SERVER_ADDRESSES"), &tmp)) {
		wxStringTokenizer tkz(tmp, _T(";"), wxTOKEN_STRTOK);
		while (tkz.HasMoreTokens()) {
			wxString token = tkz.GetNextToken();
			int pos = token.Find(L'/');
			if (pos == -1)
				continue;
			wxString prefix = token.Left(pos);

			wxString tmp = token.Mid(pos + 1);
			pos = tmp.Find(L'/');
			if (pos == -1)
				continue;

			wxString address = tmp.Left(pos);
			wxString port = tmp.Mid(pos + 1);
			ret[prefix] = std::make_pair(address, port);
		}
	}
	return ret;
}

void DrawApp::SetProbersAddresses(const std::map<wxString, std::pair<wxString, wxString> > &addresses) {
	wxConfigBase* config = wxConfigBase::Get(true);
	wxString string;
	for (std::map<wxString, std::pair<wxString, wxString> >::const_iterator i = addresses.begin();
			i != addresses.end();
			i++) {
		if (m_probers_from_szarp_cfg[i->first] != i->second)
			string += i->first + _T("/") + i->second.first + _T("/") + i->second.second + _T(";");

	}
	config->Write(_T("PROBE_SERVER_ADDRESSES"), string.Mid(0, string.Length() - 1));
	config->Flush();
	m_dbmgr->SetProbersAddresses(GetProbersAddresses());
}

DrawApp::~DrawApp() {
}


DrawGLApp::DrawGLApp() : wxGLApp(), szAppImpl() {
}

void DrawGLApp::InitSzarpDataDir() {
	return szAppImpl::InitSzarpDataDir();
}

void DrawGLApp::SetSzarpDataDir(wxString dir) {
	szAppImpl::SetSzarpDataDir(dir);
}

bool DrawGLApp::OnInit() {

	if (!wxGLApp::OnInit())
		return false;

	szAppImpl::OnInit();
	szAppImpl::SetProgPath(argv[0]);
	return true;
}

wxString DrawGLApp::GetSzarpDataDir() {
	return szAppImpl::GetSzarpDataDir();
}

void DrawGLApp::InitializeLocale(wxArrayString &catalogs, wxLocale &locale)
{
	szAppImpl::InitializeLocale(catalogs, locale);
}

void DrawGLApp::InitializeLocale(wxString catalog, wxLocale &locale)
{
	szAppImpl::InitializeLocale(catalog, locale);
}

wxString DrawGLApp::GetSzarpDir() {
	return szAppImpl::GetSzarpDir();
}

void DrawGLApp::ShowAbout(wxWindow *parent)
{
	szAppImpl::ShowAbout(parent);
}

void DrawGLApp::SetProgName(wxString str) {
	szAppImpl::SetProgName(str);
}

void DrawApp::HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event) const
{
	UDPLogger::HandleEvent( handler , func , event );
	wxApp::HandleEvent( handler , func , event );
}

