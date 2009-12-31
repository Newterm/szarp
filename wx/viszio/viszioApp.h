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
/* $Id: viszioApp.h 1 2009-11-17 21:44:30Z asmyk $
 *
 * viszio program
 * SZARP
 *
 * asmyk@praterm.com.pl
 */
 
#ifndef __VISZIOAPP_H__
#define __VISZIOAPP_H__

#include <wx/app.h>
#include "szapp.h"
#include "libpar.h"
#include "viszioFetchFrame.h"

class viszioApp : public szApp
{
	wxLocale locale;
	wxString m_server;
#ifndef MINGW32    	
	bool m_loadAll;
#endif
    bool m_deleteAll;
    bool m_loadOne;
    bool m_deleteOne;
    bool m_showAll;
    bool m_createNew;
    wxString m_configuration;
	wxString m_configurationToDelete;
	wxString m_configurationToCreate;
	szHTTPCurlClient *m_http;
public:
    virtual bool OnInit();
    int OnExit();
    bool OnCmdLineError(wxCmdLineParser&);
    bool OnCmdLineHelp(wxCmdLineParser&);
    bool OnCmdLineParsed(wxCmdLineParser&);
    void OnInitCmdLine(wxCmdLineParser&);
    void ShowConfigurations();
    bool LoadConfiguration(wxString);
#ifndef MINGW32        
    bool LoadAllConfiguration();
#endif
    bool DeleteConfiguration(wxString);
    bool DeleteAllConfigurations();
    bool CreateConfiguration(wxString);
};

#endif // __VISZIOAPP_H__
    