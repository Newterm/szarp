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

#ifndef __SZAUAPP_H__
#define __SZAUAPP_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/snglinst.h>
#endif

#include <wx/taskbar.h>

#include "szapp.h"
#include "szautaskbaritem.h"
#include "downloader.h"

#include "boost/filesystem/path.hpp"

namespace fs = boost::filesystem;

/**
 * Main application class.
 */
class SzauApp : public szApp {
public:
	fs::wpath getInstallerLocalPath();
protected:

	Downloader * m_downloader;


	SzauTaskBarItem * m_taskbar;
	/**
	 * Method called on application start.
	 */
	virtual bool OnInit();
	/**
	 * Method called on application shutdown.
	 */
	virtual int OnExit();
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

};

DECLARE_APP(SzauApp)

enum {
	ID_HELP_MENU = wxID_HIGHEST,
	ID_ABOUT_MENU,
	ID_CLOSE_APP_MENU,
	ID_INSTALL_MENU,
	ID_TIMER,
};

#endif

