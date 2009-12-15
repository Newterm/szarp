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
/* $Id: visioFetchFrame.h 1 2009-11-17 21:44:30Z asmyk $
 *
 * visio program
 * SZARP
 *
 * asmyko@praterm.com.pl
 */
 
#ifndef VISIOFETCHFRAME_H
#define VISIOFETCHFRAME_H
#include "wx/taskbar.h"
#include "httpcl.h"
#include <szarp_config.h>
#include "visioApp.h"
#include "conversion.h"
#include "fetchparams.h"
#include "serverdlg.h"
#include "visioParamadd.h"
#include "parlist.h"
#include "visioTransparentFrame.h"

#define SZ_REPORTS_NS_URI _T("http://www.praterm.com.pl/SZARP/reports")

enum
{
    PU_RESTORE = 10001,
    PU_NEW_ICON,
    PU_OLD_ICON,
    PU_EXIT,
    PU_CHECKMARK,
    PU_SUB1,
    PU_SUB2,
    PU_SUBMAIN,
};

class FetchFrame: public TransparentFrame
{
	DECLARE_EVENT_TABLE()
    szHTTPCurlClient *m_http;
    wxString m_server;
    
public:
	
    FetchFrame(wxFrame *frame, wxString server);
    ~FetchFrame();
private:
    virtual void OnClose(wxCloseEvent& event);
    virtual void OnQuit(wxCommandEvent& event);
    virtual void OnAbout(wxCommandEvent& event);
	virtual void OnFetch(wxCommandEvent& event);
};

#endif
