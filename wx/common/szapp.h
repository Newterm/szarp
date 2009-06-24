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
 * szApp - base class for all SZARP wx applications
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#ifndef __SZAPP_H__
#define __SZAPP_H__

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/cmdline.h>

#include "config.h"

class szAppImpl {
	bool has_data_dir;

	wxString m_path;

	wxString m_szarp_dir;	/* Main Szarp directory. */

	wxString m_szarp_data_dir; /* Main Szarp data directory */

	wxString m_progname;

	wxString m_version;

	wxString m_releasedate;

public:
	szAppImpl();

	bool OnInit();

	wxString GetSzarpDir(wxString path);

	void InitSzarpDataDir();

	wxString GetSzarpDir();

	void SetSzarpDataDir(wxString dir);

	wxString GetSzarpDataDir();

	void InitializeLocale(wxArrayString &catalogs, wxLocale &locale);

	void InitializeLocale(wxString catalog, wxLocale &locale);

	void ShowAbout();

	void SetProgPath(wxString path) { m_path = path; }

	void SetProgName(wxString str) { m_progname = str; };

};

/** Loads aditional Gtk resources */
class szApp: public wxApp, szAppImpl {
public:
	szApp();
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

	virtual bool OnCmdLineError(wxCmdLineParser &parser);

	virtual bool OnCmdLineHelp(wxCmdLineParser &parser);

	virtual bool OnCmdLineParsed(wxCmdLineParser &parser);

	virtual void OnInitCmdLine(wxCmdLineParser &parser);
protected:
	/** Retrives szarp data directory location*/
	void InitSzarpDataDir();

	/** Initializes loacle in a unified way */
	void InitializeLocale(wxArrayString &catalogs, wxLocale &locale);
	void InitializeLocale(wxString catalog, wxLocale &locale);

};


#endif /* __SZAPP_H__ */
