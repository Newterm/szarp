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
 * draw3 program
 * SZARP
 
 * $Id: draw.h 11 2009-07-15 09:16:00Z reksi0 $
 */

#ifndef __DRAWCTRL_H__
#define __DRAWCTRL_H__


#include <map>
#include <vector>

class DrawsObservers {
	std::vector<DrawObserver*> m_observers;

public:
	void AttachObserver(DrawObserver *observer);
	void DetachObserver(DrawObserver *observer);

	void NotifyNewData(Draw *draw, int idx);
	void NotifyCurrentProbeChanged(Draw *draw, int pi, int ni, int d);
	void NotifyEnableChanged(Draw *draw);
	void NotifyNewRemarks(Draw *draw);
	void NotifyStatsChanged(Draw *draw);
	void NotifyDrawMoved(Draw *draw, const wxDateTime& time);
	void NotifyBlockedChanged(Draw *draw);
	void NotifyDrawInfoChanged(Draw *draw);
	void NotifyDrawInfoReloaded(Draw *draw);
	void NotifyDrawSelected(Draw *draw);
	void NotifyDrawDeselected(Draw *draw);
	void NotifyPeriodChanged(Draw *draw, PeriodType period);
	void NotifyNoData(Draw *draw);
	void NotifyNoData(DrawsController *draws_controller);
	void NotifyDoubleCursorChanged(DrawsController *draws_controller);
	void NotifyFilterChanged(DrawsController *draws_controller);
	void NotifyNumberOfValuesChanged(DrawsController *draws_controller);

};

class DrawsController : public DBInquirer, public ConfigObserver {

	/**State of the Draw. Not selected params may be only in state STOP and DISPLAY.
	 * Selected param in any state*/
	enum STATE {
		/**Parameter is stoped, no queries to database are issued*/
		STOP,		
		/**Values are simply displayed*/
		DISPLAY,

		/* To enter any of the following states Draw must be selected.
		 * In any of states below user cannot change the @see m_current_time 
		 * (change cursor position) by any other mean than clicking on the
		 * draw*/


		/**Database is searched in left direction. */
		SEARCH_LEFT,
		/**Database is searched in right direction. */
		SEARCH_RIGHT,
		/**Database is searched in both directions from proposed time. */
		SEARCH_BOTH,
		/**Draw waits for data to appear on the "left side" of the cursor. */
		WAIT_DATA_LEFT,
		/**Draw waits for data to appear on the "right side" of the cursor. */
		WAIT_DATA_RIGHT,
		/**Draw waits for data to appear as close as it is possible on any side of the cursor 
		* or m_suggested_time*/
		WAIT_DATA_NEAREST,
		/**Database is searched in both directions from proposed time, left search result
		 * is choosen if data is found on the left side, otherwise time neareset to proposed
		 * time is set*/
		SEARCH_BOTH_PREFER_LEFT,
		/**Database is searched in both directions from proposed time, rithe search result
		 * is choosen if data is found on the left side, otherwise time neareset to proposed
		 * time is set*/
		SEARCH_BOTH_PREFER_RIGHT

	} m_state;

	class TimeReference {
		int m_month;
		int m_day;
		wxDateTime::WeekDay m_wday;
		int m_hour;
		int m_minute;
		int m_second;
	public:
		TimeReference(const wxDateTime &datetime);
		void Update(const DTime& time); 
		DTime Adjust(PeriodType to_period, const DTime& time);
	} m_time_reference;

	ConfigManager *m_config_manager;

	/**Indicates if search for data in left direction has been received*/
	bool m_got_left_search_response;
	/**Indicates if search for data in right direction has been received*/
	bool m_got_right_search_response;

	/**Result of search for search for data in left direction*/
	DTime m_left_search_result;
	/**Result of search for search for data in right direction*/
	DTime m_right_search_result;

	DTime m_suggested_start_time;

	DTime m_current_time;

	DTime m_time_to_go;

	int m_selected_draw;

	PeriodType m_period_type;

	int m_active_draws_count;

	DrawSet* m_draw_set;

	std::vector<Draw*> m_draws;

	std::map<std::pair<wxString, std::pair<wxString, int> >, bool> m_disabled_draws;

	DrawsObservers m_observers;

	int m_current_index;

	/**Denotes if we are working in 'double cursor mode (affects table range calculation)*/
	bool m_double_cursor;

	/**Follow last data mode*/
	bool m_follow_latest_data_mode; 

	/**Filter level*/
	int m_filter;

	wxString m_current_prefix;

	wxString m_current_set_name;

	size_t m_units_count[PERIOD_T_LAST];

	void DisableDisabledDraws();

	/** In all but WAIT* states this method does nothing. 
	 *
	 * If param is in @see WAIT_DATA_LEFT state values from the one 
	 * indexed by @see m_proposed_time are scanned in left direction.
	 * Scans stops if at first probe that is present and is not SZB_NODATA or the
	 * value for it has not yet been received. In former case param enters
	 * @see DISPLAY state and @see m_current_time is set to found probe's time. In latter
	 * case nothing happens. If no probe meeting above conditions is found param 
	 * enters @see SEARCH_LEFT state.
	 *
	 * In @see WAIT_DATA_RIGHT state method does exactly the same as in @see WAIT_DATA_RIGHT
	 * but directions are reversed.
	 *
	 * In WAIT_DATA_BOTH probes are scanned 'simultaneously' in both directions from 
	 * the probes indexed by @see m_proposed_time. Scan stop contions are the same 
	 * as in other WAIT* states. This also refers to action performed upon encountering
	 * probe that meets these cond..If no probe is found @see Draw enters @see
	 * SEARCH_BOTH state.
	 */
	void CheckAwaitedDataPresence();

	void FetchData();

	/**Issues queriers for probes in @see m_values table that are is empty state*/
	void TriggerSearch();

	/**Sends a search query to the db
	 * @param start - search start time
	 * @param end - search end time
	 * @param direction - direcrion of search*/
	void SendSearchQuery(const wxDateTime& start, const wxDateTime& end, int direction);
	
	DatabaseQuery* CreateQuery(const std::vector<time_t> &times) const;

	void EnterSearchState(STATE state, DTime search_from, const DTime& suggested_start_time);

	void EnterWaitState(STATE state);

	void EnterDisplayState(const DTime& time);

	void NoDataFound();

	void MoveToTime(const DTime &time);

	DTime ChooseStartDate(const DTime& found_time);

	void DoSet(DrawSet *set);

	void DoSwitchNextDraw(int dir);

	void DoSetSelectedDraw(int draw_to_select);

	void EnterStopState();

	void ThereIsNoData();

	void SetSelectedDraw(int draw_to_select);

	void BusyCursorSet();
public:
	DrawsController(ConfigManager *config_manager, DatabaseManager *database_manager);

	~DrawsController();
	/**If draw is not is one of searching states the response is simply ignored.
	 * Otherwise appopriate action is taken: 
	 *
	 * If we are searching left and it is response for this direction is recevied
	 * @see m_proposed_time is set to (adjusted to period) found time and Draw
	 * enters WAIT_DATA_LEFT state. In WAIT_DATA_RIGHT state this method behaves alike.
	 * In both mentionetd states if data is not found param enters DISPLAY state.
	 *
	 *
	 * If draw is in SEARCH_BOTH state and responses for both directions are received,
	 * time which is nearer the @see m_proposed_time is chosen and also if @see m_suggested_start_time
	 * is valid, attempt is made to set it as @see m_start_time. In case both search returns -1 and we
	 * are not displaying anything i.e @see m_current_time is not valid @see DrawsWidgetis is 
	 * informed that we have no data and draw is disabled.
	 * 
	 * @param response from database*/
	void HandleSearchResponse(DatabaseQuery *query);

	void HandleDataResponse(DatabaseQuery *query);

	/**Moves cursor right*/
	void MoveCursorRight(int n = 1);

	/**Moves cursor left*/
	void MoveCursorLeft(int n = 1);

	/** Just start this program and experiment with PageUp key and you'll know what this method dooes :)*/
	void MoveScreenRight();

	/** Just start this program and experiment with PageDown key and you'll know what this method dooes :)*/
	void MoveScreenLeft();

	/**Move cursor to first available data in the @see m_values table*/
	void MoveCursorBegin();

	/**Move cursor to last available data in the @see m_values table*/
	void MoveCursorEnd();


	void SelectNextDraw();

	void SelectPreviousDraw();

	void GoToLatestDate();

	void Set(DrawSet* draws);

	void Set(PeriodType period_type);

	void Set(DrawSet *set, PeriodType pt, const wxDateTime& time, int draw_to_select);

	void Set(DrawSet *set, int draw_to_select);

	void Set(PeriodType pt, const wxDateTime& time, int draw_to_select);

	void Set(int draw_to_select, const wxDateTime& time);

	void Set(const wxDateTime& time);

	void Set(int draw_no);


	int GetFilter();

	void SetFilter(int filter);


	void GetBlocked(int draw_no);

	void SetBlocked(int draw_no, bool blocked);


	bool SetDrawEnabled(int draw_no, bool enable);

	bool GetDrawEnabled(int draw_no);


	bool SetDoubleCursor(bool double_cursor);

	void SetFollowLatestData(bool follow_latest_data);

	bool GetFollowLatestData();

	bool GetDoubleCursor();


	Draw* GetDraw(size_t draw_no);

	size_t GetDrawsCount();


	void RefreshData(bool auto_refresh);

	void ClearCache();

	void SendQueryForEachPrefix(DatabaseQuery::QueryType qt);

	/**@return current param, queries for values of this param will have higher priority than queries to other params*/
	virtual DrawInfo* GetCurrentDrawInfo();

	/**@return current time, used for queries prioritization*/
	virtual time_t GetCurrentTime();

	virtual void ConfigurationWasReloaded(wxString prefix);

	virtual void SetModified(wxString prefix, wxString name, DrawSet *set);

	virtual void SetRemoved(wxString prefix, wxString name);

	virtual void SetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set);

	int GetCurrentIndex() const;

	/**method is called when response from db is received
	 * @param query object holding response from database, the same instance that was sent to the database via 
	 * @see QueryDatabase method but with proper fields filled*/
	virtual void DatabaseResponse(DatabaseQuery *query);


	Draw* GetSelectedDraw() const;

	DrawSet *GetSet();


	const PeriodType& GetPeriod();


	void AttachObserver(DrawObserver *observer);

	void DetachObserver(DrawObserver *observer);


	size_t GetNumberOfValues(const PeriodType& period);	

	void SetNumberOfUnits(size_t number_of_units);

	bool AtTheNewestValue();

	bool GetNoData();

	static const size_t max_draws_count;
};

#endif
