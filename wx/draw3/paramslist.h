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
 * draw3
 * SZARP

 *
 * $Id$
 *
 */

#ifndef __PARAMS_LIST_H__
#define __PARAMS_LIST_H__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/listctrl.h>
#endif

class DefinedParam;
class DefinedDrawsSets;
class DatabaseManager;
class PickKeyboardHandler;

class ParamsListDialog : public wxDialog {
	friend class PickKeyboardHandler;

	wxString m_prefix;

	wxListCtrl *m_param_list;

	wxTextCtrl *m_search_text;

	wxMenu *m_popup_menu;

	DefinedDrawsSets *m_def_sets;

	DatabaseManager *m_dbmgr;

	long m_selected_index;

	DatabaseManager *m_db_mgr;

	void OnAdd(wxCommandEvent &e);

	void OnRemove(wxCommandEvent &e);

	void OnEdit(wxCommandEvent &e);

	void OnSerachTextChanged(wxCommandEvent &e);

	void LoadParams();

	void OnListItemClick(wxListEvent &e);

	void OnListItemSelected(wxListEvent &e);

	void OnListItemActivated(wxListEvent &e);

	void OnClose(wxCloseEvent &e);

	void OnOKButton(wxCommandEvent &e);

	void OnCancelButton(wxCommandEvent &e);

	void OnCloseButton(wxCommandEvent &e);

	void OnHelpButton(wxCommandEvent &event);

	bool m_search_mode;

public:
	ParamsListDialog(wxWindow *parent, DefinedDrawsSets *dds, DatabaseManager *dbmgr, bool search_mode);

	void SetCurrentPrefix(wxString prefix);

	void SelectPreviousParam();

	void SelectNextParam();

	void SelectCurrentParam();

	DefinedParam* GetSelectedParam();

	DECLARE_EVENT_TABLE()

};

#endif
