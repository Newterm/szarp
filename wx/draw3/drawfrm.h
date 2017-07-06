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
 */

#ifndef __DRAWFRM_H__
#define __DRAWFRM_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/aui/aui.h>
#endif

#include <vector>

/**
 * Draw Frame class. It's responsible for remember frame size and positions.
 */
class DrawFrame : public szFrame, public SetsParamsReceivedEvent, public DBInquirer {
public:
	/**
	 * It's orginal constructor from wxFrame.
	 * @see wxwidgets documentation...
	 */
	DrawFrame(FrameManager *fm, 
			DatabaseManager *dm, 
			ConfigManager *cm, 
			RemarksHandler *remarks,
			wxWindow* parent, 
			wxWindowID id, 
			const wxString& title,
			const wxString& name,
			const wxPoint& pos = wxDefaultPosition,
			const wxSize& size = wxDefaultSize,
			long style = wxDEFAULT_FRAME_STYLE);
	/** 
	 * Method add DrawPanel 
	 */
	bool AddDrawPanel(const wxString &m_base, const wxString& set, PeriodType pt, time_t time, int selected_draw = 0);

	/**
	 * Method call for Window -> Show average type.
	 */
	void OnShowAverage(wxCommandEvent &evt); 
	/**
	 * Method call for Window -> Show interface.
	 */
	void OnShowInterface(wxCommandEvent &evt); 
	/**
	 * Method call when we used exit from MenuBar.
	 */
	void OnExit(wxCommandEvent &evt); 

	/**
	 * Refreshes data in current panel
	 */
	void OnRefresh(wxCommandEvent & event);
	
	/**
	 * Method shows Draw3 documentation
	 */
	void OnHelp(wxCommandEvent &evt);
	
	/**
	 * Method show Draw3 version
	 */
	void OnAbout(wxCommandEvent &evt);
	
	/**
	 * Method turn on IncrementalSearch widget
	 */
	void OnFind(wxCommandEvent &evt);
	
	/** Method show parameter setting widget */
	void OnSetParams(wxCommandEvent &evt); 
	
	/**
	 * Adding new defined window
	 */
	void OnAdd(wxCommandEvent &evt);

	void OnGraphsView(wxCommandEvent &e);

	/**
	 * Removing defined window
	 */
	void OnDel(wxCommandEvent &evt);

	void OnUserParams(wxCommandEvent &evt);

	void OnLanguageChange(wxCommandEvent &evt);

	void UpdatePanelName(DrawPanel *panel);
	
	/**
	 * Editing defined window
	 */
	void OnEdit(wxCommandEvent &evt);

	void OnEditSetAsNew(wxCommandEvent &evt);

	void OnImportSet(wxCommandEvent &event);

	void OnExportSet(wxCommandEvent &event);
	
	/**
	 * Saving defined window to XML file
	 */
	void OnSave(wxCommandEvent &evt);
	
	/**Shows a config dialog*/
	void OnLoadConfig(wxCommandEvent &evt);

	/**Close event handler*/
	void OnClose(wxCloseEvent &event);

	/**handles event change event sent by menu*/
	void OnFilterChange(wxCommandEvent &event);

	/**Changes type of average values displayed by current param*/
	void OnAverageChange(wxCommandEvent& event);

	/**shows @see ErrorFrame*/
	void OnErrorFrame(wxCommandEvent& event);

	/**toggles @see PieWindow visibility*/
	void OnPieWin(wxCommandEvent &event);

	/** Before calling wxFrame::Destroy() try to save defined sets (if
	 * they where modified and if user agrees)*/
	bool Destroy();

	/**Switches to next tab*/
	void SelectNextTab();

	/**Switches to previous tab*/
	void SelectPreviousTab();

	/**Toggles full screen mode*/
	void SwitchFullScreen();

	/**Reloads current config*/
	void OnReloadConfig(wxCommandEvent &event);

	void OnContextHelp(wxCommandEvent &event);

	void OnNumberOfUnits(wxCommandEvent &event);

	void OnAddRemark(wxCommandEvent &event);

	void OnFetchRemarks(wxCommandEvent &event);
		
	void OnConfigureRemarks(wxCommandEvent &event);

	void OnPrintPageSetup(wxCommandEvent& event);

	void OnShowRemarks(wxCommandEvent &e);

	void OnShowKeyboard(wxCommandEvent &e);

	void OnGoToLatestDate(wxCommandEvent &e);

	void OnSortGraph(wxCommandEvent &e);

	void OnExportDataToFile(wxCommandEvent& e);

	/**Schedules removal of a panel (at next Idle event)*/
	void RemovePanel(DrawPanel *panel);

	/**sets title of the window*/
	virtual void SetTitle(const wxString &title, const wxString &prefix);

	/**save layout*/
	void SaveLayout();

	/**load layout*/
	bool LoadLayout();

	/**load menu bar state from config*/
	void LoadMenuBarConfig(wxMenuBar *menu_bar);

	DrawPanel *GetCurrentPanel();

	void SetsParamsReceiveError(wxString error);

	void SetsParamsReceived(bool);

	void ToggleMenuBarVisbility();

	~DrawFrame();

protected:
	wxString m_name;

	/**Removes all panels that was scheduled by @see RemovePanel for removal*/
	void RemovePendingPanels();
	/**closes current tab*/
	void OnCloseTab(wxCommandEvent &command);

	wxString GetTitleForPanel(wxString title, int panel_no);

	/**Closes tab of given index
	 * @param sel index of tab to remove*/
	void CloseTab(int sel);
	
	/**Method called when user closes*/
	void OnNotebookPageClose(wxAuiNotebookEvent &event);

	/**UI event handler - Shows/hides @see SummaryWindow*/
	void OnSummaryWindowCheck(wxCommandEvent &event);

	/**Detaches panel from window and sets the panel as child of @see DrawFrame*/
	void DetachFromNotebook();

	/**Updates @see draw_panel variable and changes title of the frame**/
	void OnNotebookSelectionChange(wxAuiNotebookEvent &event);

	/**Displays jump to date window*/
	void OnJumpToDate(wxCommandEvent &event);

	/**Shows print dialog*/
	void OnPrint(wxCommandEvent &event);

	/**Shows print preview frame*/
	void OnPrintPreview(wxCommandEvent &event);

	/**Displays dialog that let user choose maximal number of printed vertical axes*/
	void OnNumberOfAxes(wxCommandEvent &event);

	/**Toggles ration window visibility*/
	void OnRelWin(wxCommandEvent &event);

	/**Shows @see XYDialog*/
	void OnXYDialog(wxCommandEvent& event);

	void OnXYZDialog(wxCommandEvent &event);

	/**Shows @see StatDialog*/
	void OnStatDialog(wxCommandEvent &event);

	/**Displays remarks frame*/
	void OnRemarks(wxCommandEvent &event);

	/** Enables/disable full screen mode. Menu event handler*/
	void OnFullScreen(wxCommandEvent &event);

	/** Toggles split cursor mode*/
	void OnSplitCursor(wxCommandEvent &event);

	void OnLatestDataFollow(wxCommandEvent &event);

	/** Idle event handler*/
	void OnIdle(wxIdleEvent &event);

	/**Copies current panel reference to a clipboard*/
	void OnCopy(wxCommandEvent &event);

	/**Pastes current panel reference from a clipboard to a current panel.*/
	void OnPaste(wxCommandEvent &event);

	/**Copies current draw param name to a clipboard*/
	void OnCopyParamName(wxCommandEvent &event);

	/**Clears cache.*/
	void OnClearCache(wxCommandEvent &event);

	/**Import draw2 windows menu event menu event*/
	void OnImportDraw2Def(wxCommandEvent &event);

	void OnProberAddresses(wxCommandEvent &event);

	void OnFetchNewNetworkParamsAndSets(wxCommandEvent &e);

	void OnSearchDate(wxCommandEvent &event);

	/** 
	 * Finds next DrawPanel with configuration given by prefix
	 * (looks in m_notebook and draw_panel).
	 * @param prefix prefix of configuration
	 * @first flag marking first search - searching is done from the
	 * beginning otherwise it continues form last search
	 */
	DrawPanel* FindNextPanel(wxString prefix, bool first);

	/** @see FrameManager*/
	FrameManager *frame_manager;

	/** @see ConfigManager*/
	ConfigManager *config_manager;
	
	/** @see DatabaseManager*/
	DatabaseManager *database_manager;

	/** Notebook holding tabs*/
	wxAuiNotebook *m_notebook;

	/** Current open draw panel*/
	DrawPanel *draw_panel;

	RemarksHandler *remarks_handler;

	/** panels pending for removal at idle event handler*/
	std::vector<DrawPanel*> panels_to_remove;

	wxMenuBar *menu_bar;

	ParamsListDialog *params_dialog;

	bool ignore_menu_events;

	bool panel_is_added;

	DECLARE_EVENT_TABLE();
};

#endif
