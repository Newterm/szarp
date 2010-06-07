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
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 * Single panel for all program widgets.
 */

#ifndef __DRAWPNL_H__
#define __DRAWPNL_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/aui/aui.h>
#endif

/**
 * This class reprezents panel with all the draws' display and control
 * widgets. It can be used as a main frame for program tab or windows.
 */

class DrawPanel : public wxPanel, public DrawObserver {
	public:
	/**
	 * @param cfg initialized configuration manager object
	 * @param defid identifier of configuration to display
	 * @param parent parent window
	 * @param id widget identifier
	 */
	DrawPanel(DatabaseManager *_db_mgr, ConfigManager *cfg, RemarksHandler *rh, wxMenuBar* menu_bar, wxString prefix, const wxString& set, PeriodType pt, time_t time, 
		wxWindow *parent, wxWindowID id, DrawFrame *_df, int selected_draw = 0);
	virtual ~DrawPanel();

	/**Displays @see IncSearch widget allowing user to switch set and draw*/
	void StartDrawSearch(); 

	/**Displays @see IncSearch widget allowing user to switch set*/
	void StartSetSearch(); 

	void StartPSC();

	/**Makes given set a current set.
	 * @param set set to select*/
	void SelectSet(DrawSet *set);
	
	/**Menu event handler, causes data refetch*/
	void OnRefresh(wxCommandEvent &evt); 

	/**Clear cache*/
	void ClearCache();

	/**Menu event handler, displays @see IncSearch widget*/
	void OnFind(wxCommandEvent &evt); 

	/**Show/hides summary window, toolbar icon event handler*/
	void OnSummaryWindow(wxCommandEvent &event);

	/**Activates/deacivates panel, if panel is deactivated all extra windows
	 * (@see RelWindow, @see PieWindow, @see SummaryWindow) are hiden. If panel
	 * is activated all extra windows that were hidding while panlel was deacivated,
	 * are shown again. */
	void SetActive(bool active);

	/**@return current configuration prefix*/
	wxString GetPrefix();

	/**@return current configuration name*/
	wxString GetConfigName();

	/**@return currently selected set*/
	DrawSet* GetSelectedSet();


	void ShowExtraWindow(bool show, const char *name, bool& show_flag);
	/**show/hides @see PieWindow
	 * @param show if true window will be shown, if false - hidden*/
	void ShowPieWindow(bool show);

	/**show/hides @see RelWindow
	 * @param show if true window will be shown, if false - hidden*/
	void ShowRelWindow(bool show);

	/**show/hides @see RelWindow
	 * @param show if true window will be shown, if false - hidden*/
	void ShowSummaryWindow(bool show);

	/**@return poiter to summary window*/
	SummaryWindow* GetSummaryWindow();

	/**Jumps to date*/
	void OnJumpToDate();

	void ShowRemarks();
	/**toggles split cursor*/
	void ToggleSplitCursor(wxCommandEvent &event);

	void SetLatestDataFollow(bool follow);

	/**Handles priting request.
	 * @param preview if true print previev dialog, if false a print dialog will be shown*/
	void Print(bool preview);

	PeriodType SetPeriod(PeriodType pt);
	
	PeriodType GetPeriod();

	size_t GetNumberOfUnits();

	void SetNumberOfUnits(size_t);

	/**Sets filter*/
	void OnFilterChange(wxCommandEvent &event);

	/**Displays a menu that allows user to change filter level*/
	void OnToolFilterMenu(wxCommandEvent &event);

	/**@return true if currently defined set is user defined*/
	bool IsUserDefined();

	/**@select sets with given title*/
	void SelectSet(wxString title);

	/**Copies window refrence to clipboard*/
	void Copy();

	/**Pastes window refrence from clipboard to current window*/
	void Paste();

	/**Get Url**/
	wxString GetUrl(bool with_infinity);

	void SetFocus();

	size_t SetNumberOfStripes();

	size_t GetNumberOfStripes();

	void GetDisplayedDrawInfo(DrawInfo **di, wxDateTime& time);

	bool Switch(wxString set, wxString prefix, time_t time, PeriodType pt = PERIOD_T_OTHER, int selected_draw = -1);

	virtual void DrawInfoChanged(Draw *d);

	virtual void FilterChanged(DrawsController *draws_ctrl);

	virtual void PeriodChanged(Draw * draw, PeriodType pt);
	protected:

	friend class DrawPanelKeyboardHandler;

	/**Creates children widgets*/
	void CreateChildren(const wxString& Set, PeriodType pt, time_t time, int selected_draw);

	void UpdateFilterMenuItem(int filter);

	/** @see DrawFrame */
	DrawFrame *df;

	/** @see InfoWidget */
	InfoWidget *iw;

	/** @see DrawsWidget */
	DrawsWidget *dw;

	/** @see DisplayTimeWidget */
	DisplayTimeWidget *dtw;

	/** @see SelectSetWidget */
	SelectSetWidget *ssw;

	/** @see SelectDrawWidget */
	SelectDrawWidget *sw;

	/** @see TimeWidget */
	TimeWidget *tw;

	/** @see IncSearch, widget for searching draws*/
	IncSearch *dinc;

	/** @see IncSearch, widget for searching sets*/
	IncSearch *sinc;

	/** @see DatabaseManager*/
	DatabaseManager *db_mgr;

	/** @see ConfigManager*/
	ConfigManager *cfg;

	/**Horiontal panel where @see InfoWidget and SelectSetWidget are located*/
	wxPanel *hpanel;

	/**@see DrawToolBar*/
	DrawToolBar *tb;

	/** current configuration title*/
	wxString prefix;

	/**Window dispalying summary values, @see SummaryWindow*/
	SummaryWindow *smw;

	RemarksHandler *rh;

	RemarksFetcher *rmf;

	/**Indicates if summary window shall be shown while panel is activaed*/
	bool smw_show;

	/**Window dispalying @see PieWindow,*/
	PieWindow *pw;

	/**Indicates if @see PieWindow shall be shown while panel is activaed*/
	bool pw_show;

	/**Window dispalying values ratio, @see RelWindow*/
	RelWindow *rw;

	/**Indicates if @see RelWindow shall be shown while panel is activaed*/
	bool rw_show;

	/**flag idicating if widget is realized - i.e. children widgets are created*/
	bool realized;

	bool active;
	
	/**Menu that allows user to choose filter level*/
	wxMenu *filter_popup_menu;

	/**This panel's menubar*/
	wxMenuBar *menu_bar;

	DrawGraphs *dg;

//#define WXAUI_IN_PANEL
#ifdef WXAUI_IN_PANEL
	/**Object managing panel's layout.*/
	wxAuiManager am;
#endif

	DECLARE_EVENT_TABLE()
};

#endif

