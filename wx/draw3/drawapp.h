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

#ifndef __DRAWAPP_H__
#define __DRAWAPP_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/glcanvas.h>
#endif

#include <map>

#include "szapp.h"
#include "singleinstance.h"
#include "szhlpctrl.h"

#include "wxlogging.h"

class DrawGLApp : public wxGLApp, private szAppImpl {
public:
	DrawGLApp();

	/** Loads resources */
	virtual bool OnInit();

	/** Returns main szarp directory - on Linux it's set during
	 * compilation, on Windows it is calculated from program path.
	 * szConv_init() must be called before calling this function!
	 * Directory is WITH slash at the end.
	 */
	wxString GetSzarpDir();

	/** Returns szarp data directory - the direcotry where szarp databases
	 * are supposed to be stored. This direcotry can be changed by the user.
	 */
	wxString GetSzarpDataDir();

	/** Sets szarp data directory*/
	void SetSzarpDataDir(wxString dir);

	/** Sets program name to display in About dialog */
	void SetProgName(wxString str);

	/** Show About dialog */
	void ShowAbout();

	virtual bool OnCmdLineError(wxCmdLineParser &parser)  { return true; }

	virtual bool OnCmdLineHelp(wxCmdLineParser &parser) { return true;}

	virtual bool OnCmdLineParsed(wxCmdLineParser &parser) { return true;}

	virtual void OnInitCmdLine(wxCmdLineParser &parser) { }
protected:
	/** Retrives szarp data directory location*/
	void InitSzarpDataDir();

	/** Initializes loacle in a unified way */
	void InitializeLocale(wxArrayString &catalogs, wxLocale &locale);

	void InitializeLocale(wxString catalog, wxLocale &locale);
};

/**
 * Main application class.
 */
class DrawApp : public DrawGLApp
{
public:
	/** 
	 * Method called on application end and on sigint.
	 */
	int OnExit();

	QueryExecutor* GetQueryExecutor() { return m_executor; };

	int* GLContextAttribs() { return m_gl_context_attribs; }

	bool GLWorks() { return m_gl_works; }

	DrawApp();

	void DisplayHelp();

	wxGLContext* GetGLContext();

	std::map<wxString, std::pair<wxString, wxString> > GetProbersAddresses();

	void SetProbersAddresses(const std::map<wxString, std::pair<wxString, wxString> >&);

	virtual bool OnCmdLineError(wxCmdLineParser &parser);
	virtual bool OnCmdLineHelp(wxCmdLineParser &parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser &parser);
	virtual void OnInitCmdLine(wxCmdLineParser &parser);

	void HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event) const;
protected:
	/**
	 * Method called on application start.
	 */
	virtual bool OnInit();

	void InitGL();
	
	virtual void StopThreads();

	/**Start IPC server that communicates with other m_instances of this application*/
	void StartInstanceServer(FrameManager *frmmgr, ConfigManager *cfgmgr);

	/**Sends a query to other runnig m_instance of this application
	 * @param topic topic of converstaion
	 * @param message contents of actual message*/
	void SendToRunningInstance(wxString topic, wxString message);
	/**
	 * Method is responsible for parsing command line. It sets
	 * program's geometry and also prints usage info.
	 * @return 0 if OK, -1 if error occured
	 */
	int ParseCommandLine();

        wxLocale locale;
                        /**< Program's locale object. */
	wxString m_base; /**< Base name. */

	wxString m_url; /**< Draw url, describing base, set, period and time to sart with*/

	bool m_url_open_in_existing;
	bool m_show_logparams;

			/** Addresses of probers servers retrived from szarp.cfg*/
	std::map<wxString, std::pair<wxString, wxString> > m_probers_from_szarp_cfg;

			/**Server listening for requests from other running m_instances of application*/
	DrawServer* m_server;

			/**<Object representing thread that executes queries*/
	QueryExecutor* m_executor;

			/**Database manager object*/
	DatabaseManager* m_dbmgr;

			/**Queries queue*/
	DatabaseQueryQueue* m_db_queue;

			/**Remarks handling object*/
	RemarksHandler* m_remarks_handler;

	ConfigManager *m_cfg_mgr;

	int x, y; /**postion and size of displayed frame*/
	int width, height;
		  /**size of frames*/

	int *m_gl_context_attribs;

	bool m_gl_works;

	wxGLContext *m_gl_context;

	bool m_just_print_version;

	/**Object that prevents more than one m_instance of program from running.*/
	szSingleInstanceChecker *m_instance;

	szHelpController *m_help; /**< Help system. */

	virtual ~DrawApp();
};
	
DECLARE_APP(DrawApp)


#endif

