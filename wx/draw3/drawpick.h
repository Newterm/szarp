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
 * Creating new defined window
 */

#ifndef _PARPICK_H
#define _PARPICK_H

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/spinctrl.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/colordlg.h>

#include "wxlogging.h"

/**
 * Widget for editing draw properties.
 */
class DrawEdit : public wxDialog {
public:
		/** Widget constructor
		 * @param color_picker color picking widget
		 */
	DrawEdit(wxWindow * parent, wxWindowID id, const wxString title, ConfigManager* cfg);

		/** Color menu item picked */
	void OnColor(wxCommandEvent & event);

		/** Changes accepted by user - OK button clicked*/
	void OnOK(wxCommandEvent & event);

	void OnCancel(wxCommandEvent & event);

	void OnScaleValueChanged(wxCommandEvent& event);

	/**Start edition of e draw*/
	int Edit(DefinedDrawInfo * draw);

	wxString GetLongName();

	wxString GetShortName();

	wxString GetBasePrefix();

	wxString GetUnit();

	double GetMin();

	double GetMax();

	int GetScale();

	double GetScaleMin();

	double GetScaleMax();

	wxColour GetColor();

	/**Cancel edition of of draw*/
	void Cancel();

	DrawInfo* GetCurrentDrawInfo();

	bool IsSummaried();

	int CreateFromScratch(wxString long_name, wxString unit_name);
private:
	void Close();

	DrawPicker * m_parent;

	/**Dialog that allows user to pick a colour*/
	wxColourDialog *m_color_dialog;

		/** Full name of draw */
	wxStaticText *m_title_label;

		/** Short name input */
	wxTextCtrl *m_short_txt;

		/** Long name input */
	wxTextCtrl *m_long_txt;

		/** Min value input */
	wxTextCtrl *m_min_input;

		/** Max value input */
	wxTextCtrl *m_max_input;

		/** Scale input */
	wxTextCtrl *m_scale_input;

		/** Min scale value input */
	wxTextCtrl *m_min_scale_input;

		/** Max scale value input */
	wxTextCtrl *m_max_scale_input;

	wxTextCtrl *m_unit_input;

	wxCheckBox *m_summary_checkbox;

	/** Min value */
	double m_min;

	/** Max value */
	double m_max;

	/** Scale value */	
	int m_scale;

	/** Min scale value */	
	double m_min_scale;	

	/** Mac scale value */	
	double m_max_scale;	

	bool m_color_set;

	DefinedDrawInfo *m_edited_draw;

		/** Button calling SimpleColorPicker
		 * @see SimpleColorPicker
		 */
	wxButton *m_color_button;

	ConfigManager *m_cfg_mgr;

	IncSearch *m_inc_search;

		/** Event table - button clicked */
	DECLARE_EVENT_TABLE();

};

/**
 * "Draw Picker" Widget - selects draw to be shown on new defined window.
 */
class DrawPicker : public wxDialog, public ConfigObserver {
public:
	DrawPicker(wxWindow* parent, ConfigManager * cfg, DatabaseManager *dbmgr);

		/** Creates (if needed) and shows popup menu */
	void OnContextMenu(wxListEvent & event);

		/** call Select @see Select */
	void OnSelected(wxListEvent & event);

		/** Move parameter up: Swap(selected, selected-1) 
		 * @see Swap*/
	void OnUp(wxCommandEvent & event);
		
		/** Move parameter down: Swap(selected, selected+1) 
		 * @see Swap*/
	void OnDown(wxCommandEvent & event);

		/** Editing all properties of draw
		 * @see Edit
		 */
	void OnContextEdit(wxCommandEvent & event);
	
		/** Set m_selected and call Edit
		 * @see Edit */
	void OnEdit(wxListEvent & event);

	void OnClose(wxCloseEvent &e);

	void OnEditParam(wxCommandEvent &e);

	void EditParam(DefinedDrawInfo *set);

	int EditDraw(DefinedDrawInfo *di, wxString prefix);

	int EditAsNew(DrawSet *set, wxString prefix);

	int EditSet(DefinedDrawSet *set, wxString prefix);
		/** Starts editing of new window. 
		 * @param prefix - prefix, of start base
		 * @return wxID_OK if set was created*/
	int NewSet(wxString prefix);

		/** Removing draw */
	void OnRemove(wxCommandEvent & event);

		/** Adding new draw */
	void OnAddDraw(wxCommandEvent & event);

		/** Adding new parameter */
	void OnAddParameter(wxCommandEvent & event);

		/** Setting draw information on list */
	void SetDraw(int index, DefinedDrawInfo * draw);

		/** Sending chosen information 
		 * @see Accept*/
	void OnOK(wxCommandEvent & event);

		/** Changes rejected by user - Cancel button clicked 
		 * temporary DefinedDrawsSet is deleted*/
	void OnCancel(wxCommandEvent & event);

	void OnHelpButton(wxCommandEvent &event);

		/** Sets color name database */
	void SetColorNames();

	wxString GetUniqueTitle(wxString user_title);

	bool FindTitle(wxString& user_title);
		/** Saves user data and cretates new configuration (@see CreateConfig()) if necessary
		 * @ returns false if user data is incorrect */
	bool Accept();

		/** Removes information about chosen draws*/
	void Clear();

	virtual void ConfigurationWasReloaded(wxString prefix);

	virtual void ParamDestroyed(DefinedParam *d);

	virtual void ParamSubstituted(DefinedParam *d, DefinedParam *p);

	wxString GetNewSetName();

	~DrawPicker();

private:
	/** changes places of two draws given by its postition */
	void Swap(int a, int b);

	/** edit draw */
	void Edit();

	/** Select draw (do not use SetSelection) */
	void Select(int i);

	/**Flag indicating if any changes has been made to defined sets*/
	bool m_modified;

	wxString m_created_set_name;

	/** @see FrameManager that this object uses to open new frames*/
	FrameManager *m_frame_mgr;

	/** Set we currently act upon.*/
	DefinedDrawSet *m_defined_set;

	/** @see ConfigManager*/
	ConfigManager *m_config_mgr;

	/** List of all draws */
	wxListCtrl *m_list;

	/** @see IncSearch*/
	IncSearch *m_inc_search;

	/** Context menu */
	wxMenu *m_context;

	/** @see DrawEdit*/
	DrawEdit *m_draw_edit;

	/** Control for editing title of set*/
	wxTextCtrl *m_title_input;

	/** Button that moves graph up*/
	wxBitmapButton *m_up;

	/** Bitmap that moves graph down*/
	wxBitmapButton *m_down;

	/** index of selected draw */
	int m_selected;

	/** editing mode flag */
	bool m_editing_existing;

	DatabaseManager* m_database_manager;

	/** Refresh after changes in defwin
	 * @see defwin */
	void RefreshData(bool update_title = true);

	void StartNewSet();

	ParamsListDialog *m_paramslist;

	wxString m_prefix;

	DECLARE_EVENT_TABLE();

};
#endif				// _PARPICK_H
