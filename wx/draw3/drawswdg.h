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
 */

#ifndef __DRAWSWDG_H__
#define __DRAWSWDG_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "config.h"

#include <wx/datetime.h>
#include <wx/dynarray.h>
#include <wx/arrimpl.cpp>

#include "cfgmgr.h"
#include "database.h"
#include "drawdnd.h"

#include "dbinquirer.h"

#ifndef NO_GSTREAMER
#include <gst/gst.h>
#endif


class DisplayTimeWidget;
class DrawPanel;
class InfoWidget;
class Draw;
class PieWindow;
class RelWindow;
class SummaryWindow;
class SelectDrawWidget;
class SelectSetWidget;
class DatabaseManager;
class ConfigManager;
class TimeWidget;
class DrawsWidget;

WX_DEFINE_ARRAY(DrawInfo*, DrawInfoPtrArray);
WX_DEFINE_ARRAY(Draw*, DrawPtrArray);

class DrawGraphs {
protected:
	DrawsWidget *m_draws_wdg;
public:
	DrawGraphs() : m_draws_wdg(NULL) {}
	void SetDrawsWidget(DrawsWidget *draws_wdg) { m_draws_wdg = draws_wdg; }
	virtual void SetDrawsChanged(DrawPtrArray draws) = 0;
	virtual void StartDrawingParamName() = 0;
	virtual void StopDrawingParamName() = 0;
	virtual void Selected(int i) = 0;
	virtual void Deselected(int i) = 0;
	virtual void Refresh() = 0;
	virtual void SetFocus() = 0;
	virtual void FullRefresh() = 0;
	virtual ~DrawGraphs() {}
};


/**This structure keeps track of displayed time. At every change of current time, 
 * depending on the period, certain fields of this structure are synced with values of this time. 
 * E.g. if @see Draw is in PERIOD_T_MONTH period fields m_day and m_month are updated, if in PERIOD_T_YEAR
 * only field m_month is changed. Then if for example period is changed from YEAR to DAY
 * current time day, hour and minute is set to values of this structure. The effect is that when user
 * is at period day at e.g. 11:10 26 September and then switches to month, changes month to November,
 * then switches back to DAY, current time is set to 11:110 26 november*/
struct TimeReference {
	int m_month;
	int m_day;
	wxDateTime::WeekDay m_wday;
	int m_hour;
	int m_minute;
};

/**
 * Widget for displaying draws (with axes and so on).
 */
class DrawsWidget : public wxEvtHandler, public DBInquirer
{
public:
	/**
	 * @param parent parent widget
	 * @param id windows identifier
	 * @param info widget for printing info about param, may be NULL
	 */
        DrawsWidget(DrawPanel* parent, ConfigManager *cfgmgr, DatabaseManager* dbmgr, DrawGraphs *graphs, SummaryWindow* swin, PieWindow *pw, RelWindow *rw, wxWindowID id, InfoWidget *info, PeriodType pt, time_t time, int selected_draw = -1);

	virtual ~DrawsWidget();

	/**Delivers response to requesting param*/
	virtual void DatabaseResponse(DatabaseQuery *query);

	/**Queryies database*/
	void QueryDatabase(DatabaseQuery *query);

	void QueryDatabase(std::list<DatabaseQuery*> &qlist);

	/**Cleans old queries from database*/
	void CleanDatabase(DatabaseQuery *query);

        /** Event handler - paints all the widget. */
        void OnPaint(wxPaintEvent& event);

	/** Sets filter level*/
	void SetFilter(int filter);

	/** Event handler - handles custom program events. */
	void OnDisplayTimerEvent(wxTimerEvent &event);

	/** Event handler - handles custom program events. */
	void OnKeyboardTimerEvent(wxTimerEvent &event);

        /** Set selected period to show. No redrawing.
         * @param period period of time to show, PERIOD_T_OTHER should not
         * be used.
	 * @param reload 1 if data is to reloaded and widget painted, 0 is 
	 * used in constructor
         */
        void SetPeriod(PeriodType period, bool force = false);

	/** Resize event handler */
	void OnSize(wxSizeEvent& WXUNUSED(event));
	
	void SetFocus();

	/** Clear cache*/
	void ClearCache();
	
	/**@return @see DrawInfo object associated with @see Draw idx*/
	DrawInfo *GetDrawInfo(int idx);

	/**
	 * API for selecting draws to draw in widget - begins new selection.
	 * Changes are applied to widget only after calling SetDrawApply().
	 */
	void SetDrawClear();
	/**
	 * API for selecting draws to draw in widget - adds draw to selection.
	 * Changes are applied to widget only after calling SetDrawApply().
	 * @param draw draw to append to selection
	 */
	void SetDrawAdd(DrawInfo* draw);
	/**
	 * API for selecting draws to draw in widget - commits changes
	 * in selected draws. Widget will be repainted.
	 */
	void SetDrawApply();
	/**
	 * Disable draw if possible. Widget is refreshed if needed.
	 * @param index index of draw to disable
	 * @return 1 if draw can be disabled, 0 if draw cannot be disabled (for 
	 * example it can be selected)
	 */
	int SetDrawDisable(size_t index);
	/**
	 * Enable draw (make it visible). Widget is repainted if needed.
	 * @param index index of draw to enable
	 */
	void SetDrawEnable(size_t index);

	/**
	 * Move selection to given draw.
	 */
	void SelectDraw(wxString name);

	/**
	 * Move selection to given draw.
	 */
	void SelectDraw(int, bool move_time = false, wxDateTime move_datetime = wxDateTime::Now() );
	
	/**
	 * Move selection to next enabled draw.
	 */
	void SelectNextDraw();
	/**
	 * Move selection to previous enabled draw.
	 */
	void SelectPreviousDraw();

	/** Move cursor n positions left
	 * @param n how many positions */ 
	
	void MoveCursorLeft(int n=1);
	/** Move cursor n positions right. 
	 * @param n how many positions */ 
	void MoveCursorRight(int n=1);


	/** This method is to notify about actions invoked with keyboard.
	 * Method doesn't have to start action - some other thing may be
	 * more important (mouse events for example). NONE actions is used
	 * when all important keyboard have been depressed. 
	 * @param action type of action requested */
	void SetKeyboardAction(ActionKeyboardType action);

	/**@return current dispaled time of current draw*/
	virtual time_t GetCurrentTime();

	/**@return param of currently selected draw*/
	virtual DrawInfo* GetCurrentDrawInfo();

	void SetDisplayTimeWidget(DisplayTimeWidget *dtw);

	/**Informs not selected @see Draw objects that displayed time range has chaged
	 * Causes widget repaint*/
	void NotifyScreenMoved(const wxDateTime &time);

	/**Informs not selected @see Draw objects that start of range of statictics calculation
	 * has moved by ginev displacement.*/
	void NotifyDragStats(int disp);

	/**Via this method selected @see Draw informs the widgets that it has no data
	 * Causes selection of next @see Daw*/
	void ParamHasNoData();

	void NotifyDoubleCursorEnabled(int idx);

	void NotifyDoubleCursorDisabled();

	/**Sets @see SelectDrawWidget*/
	void SetSelWidgets(SelectDrawWidget *seldraw, SelectSetWidget *selset, TimeWidget *timewdg);
	 
	/**blcoks/unblock draw
	 * @param index index of draw to be blocked
	 * @param block if true draw is blocked if false unbloced*/
	void BlockDraw(size_t index, bool block);

	/**@return true if draw is blocked
	 * @param index index of draw*/
	bool IsDrawBlocked(size_t index);

	/**@return pointer to selected draw object or NULL if none is selected*/
	Draw* GetSelectedDraw();

	void StopMovingCursor();

	int GetSelectedDrawIndex();

	/**@return number of currently displayed draws*/
	size_t GetDrawsCount();

	/**ask user for a date to jump, and then performes a jump*/
	void OnJumpToDate();

	/**Turns off/on double cursor mode
	 * @return true if after the operation double cursor is set, false otherwise*/
	bool ToggleSplitCursor();

	bool IsDoubleCursor();

	bool IsDrawEnabled(size_t idx);

	/**Prints/preview graphs.
	 * @param preview if true graphs will be previeved*/
	void Print(bool preview);

	/**Refreshed data
	 * @param auto_refresh refreshes data, if set to true only not no-data values are 
	 * refetched from database.*/
	void RefreshData(bool auto_refresh);

	/** Switches window */
	virtual wxDragResult SetSetInfo(wxString window, wxString prefix, time_t time, PeriodType pt, wxDragResult def);

#if 0
#ifndef NO_GSTREAMER
	void Dance();

	std::vector<float>& GetSpectrumVals();
#endif
#endif

	/**Copies window refence from clipbaord*/
	void CopyToClipboard();

	/**Paste window reference to clipboard*/
	void PasteFromClipboard();

	/**Get graph URL*/
	wxString GetUrl(bool with_infinity);

	void SwitchCurrentDrawBlock();

	void SwitchToDrawInfoUponSetChange(DrawInfo *di);

	bool IsNoData();

	void MoveScreenRight();

	void MoveScreenLeft();

	size_t GetNumberOfValues();

	bool IsDefaultNumberOfValues();

	void SetNumberOfUnits(size_t count);

	size_t GetNumberOfUnits();
protected:
	/**Changes current period
	 * @param period period to change*/
        void SwitchToPeriod(PeriodType period, const wxDateTime& current_time);

	/**Attaches @see bg_view and @see info objects to @see Draw located at given index position 
	 * in @see m_draws table
	 * @param i index of @see Draw to attach to*/
	void AttachObservers(int i);

	/**Detaches @see bg_view and @see info objects from @see Draw located at given index position 
	 * in @see m_draws table
	 * @param i index of @see Draw to detach from*/
	void DetachObservers(int i);

	/** Set timer to generate next event. */
	void SetDisplayTimer();

	/** Sets all draws in STOP state*/
	void StopAllDraws();

	/** Starts all draws*/
	void StartAllDraws();

	void MoveCursorBegin();

	void MoveCursorEnd();

	/**Switches window
	 * @param set set name to switch to
	 * @param prefix configuration prefix to switch to
	 * @param time to move to
	 * @param pt period to switch*/
	bool Switch(wxString set, wxString prefix, time_t time, PeriodType pt, int selected_draw);

	/**This metod intentionally does nothing. Common wisdom says that this reduces flickering during repaint...*/
	void OnEraseBackground(wxEraseEvent& WXUNUSED(event));

	/**
	 * Select draw number i.
	 */
	void Select(int i, bool move_time = false, wxDateTime move_datetime = wxDateTime::Now());

	/** Internal array of draws to display. */
	DrawPtrArray m_draws;
	/** Array of not commited draws to display. */
	DrawInfoPtrArray m_draws_proposed;

	DrawGraphs* m_graphs;
	/**@see ConfigManager*/
	ConfigManager *m_cfg;

	/**Parent panel*/
	DrawPanel *m_parent;

	bool m_widget_freshly_created;

	size_t m_units_count[PERIOD_T_LAST];

	static const size_t default_units_count[PERIOD_T_LAST];

	/** widget for printing info about selected draw */
	InfoWidget *info;
	
        PeriodType period;      /**< Type of time period to show. */

	int m_selected_draw;	/**< Index of selected draw, -1 means no draw is selected */
				/**< Selected date */
	int m_action;		/**< Action to perform repeatedly (with timer generated 
				  events) after widget refreshing. */

	int m_filter;		/**< Current filter*/

	DisplayTimeWidget* m_displaytime;
				/**< Pointer to widget for displaying time. */

    	wxTimer* m_display_timer;
	wxTimer* m_keyboard_timer;

	/**Window displaying summary values*/
	SummaryWindow* m_summ_win;

	/**Window displaying pie draw*/
	PieWindow* m_pie_win;

	/**Window displaying values ratio*/
	RelWindow* m_rel_win;

	/**@see SelectDrawWidget*/
	SelectDrawWidget* m_seldraw;

	/**@see SelectSetWidget*/
	SelectSetWidget* m_selset;

	/**@see TimeWidget*/
	TimeWidget* m_timewdg;

	/**Flag denotes if no data is present in any of currently displayed draw*/
	bool m_nodata;

	/**@see TimeReference*/
	TimeReference m_reference;


	/**Date to switch to while changing darws set*/
	wxDateTime m_switch_date;

	/**Period to switch to while changing period*/
	PeriodType m_switch_period;

	/**draw to select while changing period*/
	int m_switch_selected_draw;

	/**draw to select while changing period*/
	DrawInfo *m_switch_selected_draw_info;

	std::map<std::pair<wxString, std::pair<wxString, int> >, bool> m_disabled_draws;

	DECLARE_EVENT_TABLE()

};

#endif

