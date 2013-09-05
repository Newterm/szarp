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
#define AUTO_LANGUAGE wxT("auto")
#define DEFAULT_LANGUAGE wxLocale::GetLanguageInfo(wxLocale::GetSystemLanguage() == wxLANGUAGE_UNKNOWN ? wxLANGUAGE_ENGLISH_UK : wxLocale::GetSystemLanguage())->CanonicalName.Left(2)


class szAppConfig {
public:
	virtual wxString GetSzarpDataDir() = 0;
	virtual wxString GetSzarpDir() = 0;
	virtual ~szAppConfig() {};
};

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

	void ShowAbout(wxWindow *parent = NULL);

	void SetProgPath(wxString path) { m_path = path; }

	void SetProgName(wxString str) { m_progname = str; };

};

/** Loads aditional Gtk resources */
template<class APP=wxApp> class szApp: public APP, public szAppConfig, szAppImpl  {
public:
	szApp() : APP(), szAppImpl() { }

	void SetSzarpDataDir(wxString dir) { szAppImpl::SetSzarpDataDir(dir); }

	virtual bool OnInit() {
		if (!APP::OnInit())
			return false;
		szAppImpl::SetProgPath(APP::argv[0]);
		szAppImpl::OnInit();
		return true;
	}

	virtual wxString GetSzarpDir() {
		return szAppImpl::GetSzarpDir();
	}

	virtual wxString GetSzarpDataDir() {
		return szAppImpl::GetSzarpDataDir();
	}

	virtual void ShowAbout()
	{
		szAppImpl::ShowAbout();
	}

	virtual void SetProgName(wxString str) {
		szAppImpl::SetProgName(str);
	}

	virtual bool OnCmdLineError(wxCmdLineParser &parser) {
		return APP::OnCmdLineError(parser);
	}

	virtual bool OnCmdLineHelp(wxCmdLineParser &parser) {
		return APP::OnCmdLineHelp(parser);
	}

	bool OnCmdLineParsed(wxCmdLineParser &parser){
		return APP::OnCmdLineParsed(parser);
	}

	virtual void OnInitCmdLine(wxCmdLineParser &parser) {
		APP::OnInitCmdLine(parser);
		parser.AddSwitch(_("Dprefix"), wxEmptyString, _T("database prefix"));
	}

protected:
	/** Retrives szarp data directory location*/
	void InitSzarpDataDir() { return szAppImpl::InitSzarpDataDir(); } 

	void InitializeLocale(wxArrayString &catalogs, wxLocale &locale)
	{
		szAppImpl::InitializeLocale(catalogs, locale);
	}

	void InitializeLocale(wxString catalog, wxLocale &locale)
	{
		szAppImpl::InitializeLocale(catalog, locale);
	}

};


#endif /* __SZAPP_H__ */
