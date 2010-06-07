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
#include <wx/dnd.h>

#ifndef NO_GSTREAMER
#include <gst/gst.h>
#endif

typedef std::vector<Draw*> DrawPtrArray;

class DrawGraphs : public DrawObserver {
protected:
	DrawsWidget *m_draws_wdg;
public:
	DrawGraphs() : m_draws_wdg(NULL) {}
	void SetDrawsWidget(DrawsWidget *draws_wdg) { m_draws_wdg = draws_wdg; }
	virtual void StartDrawingParamName() = 0;
	virtual void StopDrawingParamName() = 0;
	virtual void SetFocus() = 0;
	virtual ~DrawGraphs() {}
};

/**
 * Widget for displaying draws (with axes and so on).
 */
class DrawsWidget : public wxEvtHandler
{
	/**@see DrawGraphs*/
	DrawGraphs* m_graphs;

	/**@see ConfigManager*/
	ConfigManager *m_cfg;

	/**Parent panel*/
	DrawPanel *m_parent;

	/**Draws controller*/
	DrawsController* m_draws_controller;

	int m_action;		/**< Action to perform repeatedly (with timer generated 
				  events) after refreshing. */
    	wxTimer* m_display_timer;

	int m_display_timer_events;

	wxTimer* m_keyboard_timer;

	/**Remarks fetcher*/
	RemarksFetcher* m_remarks_fetcher;
public:
	/**
	 * @param parent parent widget
	 * @param id windows identifier
	 * @param info widget for printing info about param, may be NULL
	 */
        DrawsWidget(DrawPanel* parent, ConfigManager *cfgmgr, DatabaseManager* dbmgr, DrawGraphs *graphs, RemarksFetcher *rf);

	virtual ~DrawsWidget();

	/**Switches window
	 * @param set set name to switch to
	 * @param prefix configuration prefix to switch to
	 * @param time to move to
	 * @param pt period to switch*/
	bool SetSet(wxString set, wxString prefix, time_t time, PeriodType pt, int selected_draw);

	/** Sets filter level*/
	void SetFilter(int filter);

        /** Set selected period to show. No redrawing.
         * @param period period of time to show, PERIOD_T_OTHER should not
         * be used.
	 * @param reload 1 if data is to reloaded and widget painted, 0 is 
	 * used in constructor
         */
        void SetPeriod(PeriodType period);

	DrawSet* GetCurrentDrawSet();

	wxDateTime GetCurrentTime();

	DrawInfo* GetCurrentDrawInfo();

	bool GetNoData();

	void SetFocus();

	/** Clear cache*/
	void ClearCache();
	
	/**@return @see DrawInfo object associated with @see Draw idx*/
	DrawInfo *GetDrawInfo(int idx);

	/**
	 * Disable draw if possible. Widget is refreshed if needed.
	 * @param index index of draw to disable
	 * @return 1 if draw can be disabled, 0 if draw cannot be disabled (for 
	 * example it can be selected)
	 */
	bool SetDrawDisable(size_t index);

	/**
	 * Enable draw (make it visible). Widget is repainted if needed.
	 * @param index index of draw to enable
	 */
	void SetDrawEnable(size_t index);

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
	
	void MoveCursorLeft(int n = 1);

	/** Move cursor n positions right. 
	 * @param n how many positions */ 
	void MoveCursorRight(int n = 1);

	/** This method is to notify about actions invoked with keyboard.
	 * Method doesn't have to start action - some other thing may be
	 * more important (mouse events for example). NONE actions is used
	 * when all important keyboard have been depressed. 
	 * @param action type of action requested */
	void SetKeyboardAction(ActionKeyboardType action);

	/**Show remarks*/
	void ShowRemarks(int index);

	/**blcoks/unblock draw
	 * @param index index of draw to be blocked
	 * @param block if true draw is blocked if false unbloced*/
	void BlockDraw(size_t index, bool block);

	/**@return true if draw is blocked
	 * @param index index of draw*/
	bool GetDrawBlocked(size_t index);

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

	bool GetDoubleCursor();

	bool DoubleCursorSet(bool enable);

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

	/**Copies window refence from clipbaord*/
	void CopyToClipboard();

	/**Paste window reference to clipboard*/
	void PasteFromClipboard();

	/**Get graph URL*/
	wxString GetUrl(bool with_infinity);

	void SwitchCurrentDrawBlock();

	void MoveScreenRight();

	void MoveScreenLeft();

	size_t GetNumberOfValues();

	bool IsDefaultNumberOfValues();

	void SetNumberOfUnits(size_t count);

	size_t GetNumberOfUnits();

	void SetSet(DrawSet *set, DrawInfo* info);

	void SetSet(DrawSet *set);

	/** Event handler - handles custom program events. */
	void OnDisplayTimerEvent(wxTimerEvent &event);

	/** Event handler - handles custom program events. */
	void OnKeyboardTimerEvent(wxTimerEvent &event);

	void AttachObserver(DrawObserver *draw_observer);

	void DetachObserver(DrawObserver *draw_observer);

	DrawsController* GetDrawsController();
protected:
	/** Set timer to generate next event. */
	void SetDisplayTimer();

	/** Sets all draws in STOP state*/
	void StopAllDraws();

	/** Starts all draws*/
	void StartAllDraws();

	void MoveCursorBegin();

	void MoveCursorEnd();

	DECLARE_EVENT_TABLE()

};

#endif
