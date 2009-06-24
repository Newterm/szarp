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

#ifndef __SCCAPP_H__
#define __SCCAPP_H__

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
#include <wx/snglinst.h>
#endif

#include "sccmenu.h"
#include "sccframe.h"
#include "sccipc.h"
#include "szapp.h"

/**
 * Main application class.
 */
class SCCApp: public szApp
{
public:
	/**Genereates new SCCMenu and provides it to SCCFrame*/
	void ReloadMenu();

	virtual void OnInitCmdLine(wxCmdLineParser &parser);

	virtual bool OnCmdLineError(wxCmdLineParser &parser);

	virtual bool OnCmdLineHelp(wxCmdLineParser &parser);

	virtual bool OnCmdLineParsed(wxCmdLineParser &parser);
protected:
	/**
	 * Method called on application start.
	 */
	virtual bool OnInit();
	/**
	 * Method called on application shutdown.
	 */
	virtual int OnExit();
	/**
	 * Method is responsible for parsing command line. It sets
	 * program's geometry and also prints usage info.
	 * @param argc - command line paramters count
	 * @param argv - command line paramters list
	 * @return 0 if OK, -1 if error occured
	 */
	int ParseCommandLine(int argc,wxChar** argv);
	int x, y;	/**< Program's window position. */
	/** object resposible for handling locale */
	wxLocale locale;
	/** 
	 * Used to verify if other scc instance is running.
	 */
	wxSingleInstanceChecker* app_instance;
#ifndef MINGW32
	/** 
	 * Used to verify if other scc instance is running
	 * on the same display. We need to check this so we don't
	 * show "user is logged more than once" message if he or she,
	 * is not.
	 */
	wxSingleInstanceChecker* app_instance2;
#endif
	/**
	 * Creates application main menu
	 */
	SCCMenu* CreateMainMenu();

	/**Pointer to main application frame*/
	SCCTaskBarItem* taskbar_item;

	/**Poniter to SCCServer*/
	SCCServer *server;

	bool reload_menu;
};

DECLARE_APP(SCCApp)


#endif

