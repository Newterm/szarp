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
 * Dialog for selecting SZARP raporter server.
 *

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 * 
 * $Id$
 */

#ifndef __SERVERDLG_H__
#define __SERVERDLG_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <wx/progdlg.h>
#include <wx/timer.h>
#include <wx/thread.h>
#include "szarp_config.h"
#include "fetchparams.h"

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

/** Class for picking server address and port. */
class szServerDlg {
public:
	/** Displays dialog for selecting server address and port. Dialog
	 * contains checkbox for saving specified server as default. If
	 * default server is given empty, the saved one is used. If user press
	 * Cancel button, empty string is returned.
	 * @param default default server specification, if wxEmptyString is
	 * used, the saved one is used as default
	 * @param always_show if true dialog is always shown (even if default
	 * is specified), if false the default one is returned or empty string
	 * if no default is specified
	 * @return server address and port (no validation!), may be
	 * wxEmptyString
	 */
	static wxString GetServer(wxString def, wxString progname, bool always_show = true, wxString configuration = _T("ServerDlg"));
	static TSzarpConfig * GetIPK(wxString server,szHTTPCurlClient *m_http);
	static bool GetReports(wxString server, szHTTPCurlClient *m_http, wxString &title, wxArrayString &reports);
};

/** Class for loading ipk from remote server. */
class GetIPKThread: public wxThread 
{
	/** name of server */
	wxString m_server;
	/** szarp configuration */
	TSzarpConfig **m_ipk;
	/** http client */
	szHTTPCurlClient **m_http;
	/** main thread function, it obtaines ipk from a given server. 
		This function can change http and ipk parameters passed to
		the constructor.
	*/
	virtual void* Entry();
public:
	/** Constructor of the GetIPKThread class
	* @param server name of the server to be explored
	* @param http address of a http client 
	* @param ipk address of a szarp configuration 
	*/
	GetIPKThread(wxString path, szHTTPCurlClient **http, TSzarpConfig **ipk);
};

#endif /* __SERVERDLG_H__ */

