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
/* $Id: viszioFetchFrame.h 1 2009-11-17 21:44:30Z asmyk $
 *
 * viszio program
 * SZARP
 *
 * asmyk@praterm.com.pl
 */
 
#ifndef __VISZIOFETCHFRAME_H__
#define __VISZIOFETCHFRAME_H__
#include "wx/taskbar.h"
#include "httpcl.h"
#include <szarp_config.h>
#include "viszioApp.h"
#include "conversion.h"
#include "fetchparams.h"
#include "serverdlg.h"
#include "viszioParamadd.h"
#include "parlist.h"
#include "viszioTransparentFrame.h"

#define SZ_REPORTS_NS_URI _T("http://www.praterm.com.pl/SZARP/reports")

/**
 * Viszio transparent frame with event fetch handling.
 */
class FetchFrame: public TransparentFrame
{
	DECLARE_EVENT_TABLE()
	/** HTTP client */
    szHTTPCurlClient *m_http;
    /** name of server */
    wxString m_server;
    
public:
	/** FetchFrame constructor.
	 * @param frame parent window 
	 * @param server name of server
	 * @param http http client
	 * @param paramName name of parameter
	 */ 
    FetchFrame(wxFrame*, wxString, szHTTPCurlClient*, wxString paramName);
    ~FetchFrame();
private:
    /** Called on fetching parameters */
	virtual void OnFetch(wxCommandEvent& event);
};

#endif
