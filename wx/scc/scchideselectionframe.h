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

 * Adam Smyk asmyk@praterm.pl
 *
 * $Id: scchideselectionframe.h 1 2009-06-24 15:09:25Z asmyk $
 */

#ifndef _SCCHIDESELECTIONFRAME_H_
#define _SCCHIDESELECTIONFRAME_H_

#ifdef __WXMSW__
#define _WIN32_IE 0x600
#endif

#include "config.h"
#include "version.h"
#include <wx/platform.h>

#include <math.h>

#ifndef MINGW32
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <map>
#include <set>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/wxprec.h>
#include <wx/progdlg.h>
#include <wx/cmdline.h>
#include <wx/datetime.h>
#include <wx/statline.h>
#include <wx/config.h>
#include <wx/snglinst.h>
#include <wx/thread.h>
#include <wx/taskbar.h>
#include <wx/wizard.h>
#include <wx/image.h>
#include <wx/ipc.h>
#include <wx/tokenzr.h>
#include <wx/dir.h>
#include <wx/xml/xml.h>
#include <wx/log.h>
#else
#include <wx/wx.h>
#endif

#include "szapp.h"
#include "szhlpctrl.h"
#include "cfgnames.h"

#ifdef MINGW32
#include "mingw32_missing.h"
#endif


enum {
	ID_SELECTION_FRAME_OK_BUTTON,
	ID_SELECTION_FRAME_CANCEL_BUTTON
	};

/**This class lets user to show only chosen databases.*/
class SCCSelectionFrame : public wxDialog
{
	DECLARE_EVENT_TABLE();

	/**OK button press handler. Saves currently selected databases*/
	void OnOKButton(wxCommandEvent& event);
	/**Cancel button press handler. Hides dialog*/
	void OnCancelButton(wxCommandEvent& event);
	/**CheckListBox for selecting database to hide*/
	wxCheckListBox* m_databases_list_box;
	/**List of hiden databases*/
	wxArrayString m_hidden_databases;
	/**List of local databases*/
	wxArrayString m_databases;
	/**List of local databases*/
	std::map<wxString, wxString> m_database_server_map;
	/**Config titles*/
	ConfigNameHash m_config_titles;
	/**Loads selected databases from configuration*/
	void LoadConfiguration();
	/**Saves selected databases in configuration*/
	void StoreConfiguration();
	/**Gets list of local databases*/
	void LoadDatabases();
public:
	/**Gets list of hidden databases*/
	wxArrayString GetHiddenDatabases();
	/**Default constructor*/
	SCCSelectionFrame();
};

#endif
