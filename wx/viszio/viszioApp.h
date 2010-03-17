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

//#include <wx/app.h>
#include "szapp.h"
#include "libpar.h"
#include "viszioFetchFrame.h"

/**
 * Viszio application class.
 */
class viszioApp : public szApp
{
	wxLocale locale;	/**< Locale object */
	wxString m_server;	/**< Server name */
#ifndef MINGW32    	
	bool m_loadAll;		/**< True if all configuration will be loaded */
#endif
    bool m_deleteAll;	/**< True if all configuration will be deleted */
    bool m_loadOne;		/**< True if one configuration will be loaded */
    bool m_deleteOne;	/**< True if one configuration will be deleted */
    bool m_showAll;		/**< True if all configuration will be shown */
    bool m_createNew;	/**< True if a new configuration will be created */
    wxString m_configuration;	/**< Configuration name */
	szHTTPCurlClient *m_http;	/**< HTTP client */
public:
    virtual bool OnInit();
    int OnExit();
	/** Invoked when command line error occured */
    bool OnCmdLineError(wxCmdLineParser&);
    /** Invoked when command line help (-h or --help) option is specified */
    bool OnCmdLineHelp(wxCmdLineParser&);
    /** Command line parsing */
    bool OnCmdLineParsed(wxCmdLineParser&);
    /** Initialization of command line parser */
    void OnInitCmdLine(wxCmdLineParser&);
    /** Shows all available configurations */
    void ShowConfigurations();
	/** Load configuration
	 * @param configurationName configuration name 
	 * @param true if first availabale configuration will be loaded and false if specified configuration will be loaded
	 */    
	 bool LoadConfiguration(wxString configurationName, bool defaultLoading = false);
#ifndef MINGW32        
	/** Load all configurations */    
    bool LoadAllConfiguration();
#endif
	/** Delete configuration
	 * @param configurationName configuraton name 
	 */    
    bool DeleteConfiguration(wxString);
	/** Delete all configurations */    
    bool DeleteAllConfigurations();
	/** Create configuration
	 * @param configurationName configuraton name 
	 */        
    bool CreateConfiguration(wxString);
    /** Load first valid configuration from configuration file
	 * @return true if any configuration has been valid
	 */        
    bool LoadFirstConfiguration();
private:
    /** Gets configuration name from dialog
	 * @return name of new configuration
	 */        
    wxString GetConfigurationName();
};

#endif // __VISZIOAPP_H__

