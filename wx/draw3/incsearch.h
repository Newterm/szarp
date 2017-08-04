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
 * incsearch - Incremental Search - engine & widget 
 * SZARP

 * Michal Blajerski <nameless@praterm.com.pl>
 *
 * $Id$
 */

#ifndef _INCSEARCH_H
#define _INCSEARCH_H

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <map>
#include <vector>

#include <wx/textctrl.h>
#include <wx/listctrl.h>

#include "sprecivedevnt.h"
#include "seteditctrl.h"

/**
 * Widget "Incremental Search" for looking params and windows.
 */
class IncSearch : public wxDialog, public ConfigObserver, public SetsParamsReceivedEvent, public SetEditControl {
	 DECLARE_DYNAMIC_CLASS(IncSearch)
public:
	/**
	 * Two step constructor???
	 * @see wxDialog documentation for more details
	 */
	IncSearch() : wxDialog() {};
	/**
	 * Incremental Search widget constructor
	 * @param cfg configuration manager object
	 * @param confname identifier of configuration used by current DrawPanel
	 * @param parent parent window
	 * @param id window id
	 * @param title title of incremetal search window
	 */
	IncSearch(ConfigManager *cfg, RemarksHandler *remarks_handler, const wxString& confname, wxWindow* parent, wxWindowID id, 
		const wxString& title, bool window_search = false, bool show_defined = false, bool conf_pick = true, DatabaseManager *db_mgr = NULL);
	/**
	 * Method call when we press Ok button
	 */
	void OnOk(wxCommandEvent &event);

	/**List item activation (i.e. double click) event handler*/
	void OnActivated(wxListEvent& event);
	/**
	 * Method call when we press Cancel button
	 */
	void OnCancel(wxCommandEvent &evt);
	/**
	 * Close event handler
	 */
	void OnClose(wxCloseEvent &evt);

	void OnEditMenu(wxCommandEvent &evt);

	void OnRemoveMenu(wxCommandEvent &evt);
	
	/**
	 * Method call when we choose DB - loads params from chosen DB
	 */
	void OnChoice(wxCommandEvent &evt);

	void OnHelpButton(wxCommandEvent &event);

	/**
	 * Method call when we press Reset button.
	 * Reload list of params and windows and clear input_text
	 */
	void OnReset(wxCommandEvent &evt);
	
	/**
	 * Method call when text in entered.
	 * Shows list of matching windows only
	 */
	void OnSearch(wxCommandEvent &event);
	/**
	 * Method call when we press enter.
	 */
	void OnEnter(void);
	
	/**
	 * Show or hide configuration picker.
	 */ 
	void ShowConfPicker(bool show);

	/**
	 * Gets DrawInfo about selected draw.
	 * @param prev_draw pointer to index of previous draw (-1 to start)
	 */
	DrawInfo* GetDrawInfo(long* prev_draw, DrawSet** set = NULL);

	/**Displayed item*/
	struct Item {
		/**if id requals -1 this filed hold pointer to DrawInfo this item refers*/
		DrawInfo *draw_info;

		/**Draw set*/
		DrawSet  *draw_set;
		
		/** @return name: drawname + " - Window" + drawset 
		 * or just set if in window search mode */
		wxString GetName()
		{
			if(draw_info == NULL)
				return draw_set->GetName();
			else
				return draw_info->GetName()
					 + wxT(" [") + draw_info->GetShortName() + wxT("] ")
					 + wxT(" (") + draw_info->GetParamName() + wxT(") ")
					 + _(" - Window ") + draw_set->GetName();
		}
	};

	virtual void ConfigurationWasReloaded(wxString prefix);

	/**selects next draw from the list*/
	void SelectNextDraw();

	/**selects previous draw from the list*/
	void SelectPreviousDraw();

	/**selects previous draw from the list*/
	void GoPageDown();

	/**selects previous draw from the list*/
	void GoPageUp();

	/**changes current config*/
	void SetConfigName(wxString cn);

	/**Updates widget if prefix matches currently selected prefix*/
	virtual void SetRemoved(wxString prefix, wxString name);

	/**Updates widget if prefix matches currently selected prefix*/
	virtual void SetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set);

	/**Updates widget if prefix matches currently selected prefix*/
	virtual void SetModified(wxString prefix, wxString name, DrawSet *set);

	/**Updates widget if prefix matches currently selected prefix*/
	virtual void SetAdded(wxString prefix, wxString name, DrawSet *set);

	void OnWindowCreate(wxWindowCreateEvent &event);

	void OnShow(wxShowEvent &e);

	void OnRightItemClick(wxListEvent& event);

	void FinishWindowCreation();

	void StartWith(DrawSet *set);

	void StartWith(DrawSet *set, DrawInfo* info);

	WX_DEFINE_ARRAY(Item*, ItemsArray);

	DrawSet* GetSelectedSet();

	void SetInsertUpdateError(wxString error);

	void SetInsertUpdateFinished(bool ok);

	void SetsParamsReceiveError(wxString error);

	void SetsParamsReceived(bool);

private:
	void SyncToStartDraw();

	void SelectEntry(wxString string_to_select);

	DrawSet* m_start_set;
	DrawInfo* m_start_draw_info;

	DatabaseManager *db_mgr;

	wxBoxSizer *top_sizer;

	wxBoxSizer *input_sizer;
	wxBoxSizer *button_sizer;

	/**Class necessary for virtual list control style*/
	class ListCtrl : public wxListView {
		/**pointer to item_array in @see IncSearch*/
		ItemsArray* m_cur_items;
		public:
		ListCtrl(ItemsArray *array, wxWindow *parent, int id, const wxPoint& position, const wxSize& size, long style);
		/**@return item's text*/
		virtual wxString OnGetItemText(long item, long column) const;
		void SetItems(ItemsArray* cur_items) {
			m_cur_items=cur_items;
		}
	};

	ListCtrl *item_list; /**< wxListBox contains all params and windows */

	wxTextCtrl *input_text; /**< wxTextCtrl */

	wxButton *reset_button;

	wxButton *ok_button;

	wxButton *close_button;

	wxButton *help_button;

	wxChoice *db_picker;

	/* Array of all items in configuration */
	ItemsArray items_array;

	/* Array of items matching searched text */
	ItemsArray* m_cur_items;

	/**Updates elements in @see ListCtrl*/
	void UpdateItemList();

	/** @return full names of all available configurations*/
	wxArrayString GetConfNames();
	
	/** @return prefix of given configuration*/
	wxString GetConfPrefix(wxString name);
	
	/** Sets column width */
	void SetColumnWidth();

	/** Window (not draw) search mode */
	bool m_window_search;

	/** Flag indicating if defined params belonging to configuration shaal be shown*/
	bool m_show_defined;

	RemarksHandler *m_remarks_handler;

	/** Fills widget with values that match current search criteria*/
	void Search();

	void AddWindowItems(SortedSetsArray *sorted);

	void AddDrawsItems(SortedSetsArray *array);

	virtual ~IncSearch();

	bool window_created;

	long selected_index;

protected:

	/**
	 * Method Load draws and windows.
	 */
	void LoadParams();
	
	ConfigManager *cfg; /**< ConfigManager */
	wxString confid; /**< Configuration id */

	/**
	 * Button Events
	 */
	DECLARE_EVENT_TABLE()	  

};
#endif // _INCSEARCH_H
