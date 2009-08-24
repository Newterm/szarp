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
/* * draw3
 * SZARP

 *
 * $Id$
 */


#ifndef __CFGDLG_H__
#define __CFGDLG_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "cfgnames.h"

#include <wx/statline.h>

class ConfigManager;

/** Window that lets users choose configuration to open*/
class ConfigDialog : public wxDialog {
	/**configurations */
	ConfigNameHash cfgs;

	/**Widget holding list of configurations*/
	wxListBox* m_cfg_list;

	/**Current list of prefixes*/
	wxArrayString m_cfg_prefixes;

	bool m_show_user_defined_set;

	wxTextCtrl* m_search;

	void UpdateList();

	wxString userDefinedPrefix;

	public:
	ConfigDialog(wxWindow *parent, const ConfigNameHash& _cfgs, const wxString _userDefinedPrefix);

	int ShowModal();

	void ShowUserDefinedSets(bool show);

	/*Loads configuration choosen by a user*/
	virtual void OnOK(wxCommandEvent &event);

	/*Loads configuration choosen by a user*/
	virtual void OnSearchText(wxCommandEvent &event);

	/**Hides window*/
	virtual void OnCancel(wxCommandEvent &event);

	/**Hides or destroys window*/
	void OnClose(wxCloseEvent& event);

	void SelectNextConfig();

	void SelectPreviousConfig();

	wxString GetSelectedPrefix();

	wxString GetSelectedTitle();

	void SelectCurrentConfig();

	static bool SelectDatabase(wxString &database, wxArrayString* hidden_databases = 0);

	DECLARE_EVENT_TABLE()

};
#endif
