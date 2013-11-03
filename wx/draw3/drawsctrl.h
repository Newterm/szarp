/* SZARP: SCADA software 

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
	void NotifyDrawsSorted(DrawsController *draws_controller);
	void NotifyAverageValueCalculationMethodChanged(Draw *draw);

};

class DrawsController : public DBInquirer, public ConfigObserver {

	enum STATE {
		/**Parameter is stoped, no queries to database are issued*/
		STOP = 0,
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
		SEARCH_BOTH_PREFER_RIGHT,

		FIRST_UNUSED_STATE_ID

	};

	class State {
	protected:
		DrawsController *m_c;
	public:
		void SetDrawsController(DrawsController *c);
		
		virtual void HandleDataResponse(DatabaseQuery *query);

		virtual void HandleSearchResponse(DatabaseQuery *query);

		virtual void MoveCursorBegin();

		virtual void MoveCursorEnd();

		virtual void MoveCursorLeft(int n);

		virtual void MoveCursorRight(int n);

		virtual void MoveScreenLeft();

		virtual void MoveScreenRight();

		virtual void GoToLatestDate();

		virtual void NewDataForSelectedDraw();

		virtual void SetNumberOfUnits();

		virtual void Reset();

		virtual const DTime& GetStateTime() const = 0;

		virtual void Enter(const DTime& time) = 0;

		virtual void ParamDataChanged(TParam* param);

		virtual void Leaving() {};

		virtual ~State() {};

	};

	class StopState : public State {
		DTime m_stop_time;
	public:
		void Reset();

		void SetNumberOfUnits();

		virtual void Enter(const DTime& time);

		const DTime& GetStateTime() const;

		void ParamDataChanged(TParam* param) {}
	};

	class DisplayState : public State {
	public:
		void MoveCursorBegin();

		void MoveCursorEnd();

		void MoveCursorLeft(int n);

		void MoveCursorRight(int n);

		void MoveScreenLeft();

		void MoveScreenRight();

		void SetNumberOfUnits();
	
		void Reset();

		void GoToLatestDate();

		void Enter(const DTime& time);

		void Leaving();

		const DTime& GetStateTime() const;
	};

	class WaitState : public State {
	protected:
		DTime m_time_to_go;
	public:
		void Reset();
		void NewDataForSelectedDraw();
		virtual void Enter(const DTime& time);
		virtual void CheckForDataPresence(Draw *draw) = 0;
		const DTime& GetStateTime() const;
		void ParamDataChanged(TParam* param) {}
	};

	class WaitDataLeft : public WaitState {
	public:
		void CheckForDataPresence(Draw *draw);
	};

	class WaitDataRight : public WaitState {
	public:
		void CheckForDataPresence(Draw *draw);
	};

	class WaitDataBoth : public WaitState {
	public:
		void CheckForDataPresence(Draw *draw);
	};

	class SearchState : public State {
	protected:
		DTime m_search_time;
		/**Sends a search query to the db
		 * @param start - search start time
		 * @param end - search end time
		 * @param direction - direcrion of search*/
		void SendSearchQuery(const wxDateTime& start, const wxDateTime& end, int direction);
	public:
		const DTime& GetStateTime() const;
		void Reset();
		void HandleSearchResponse(DatabaseQuery *query);
		virtual void HandleLeftResponse(wxDateTime& time);
		virtual void HandleRightResponse(wxDateTime& time);
		void ParamDataChanged(TParam* param) {}
	};

	class SearchLeft : public SearchState {
	public:
		virtual void Enter(const DTime& time);
		virtual void HandleLeftResponse(wxDateTime& time);
	};

	class SearchRight : public SearchState {
	public:
		virtual void Enter(const DTime& time);
		virtual void HandleRightResponse(wxDateTime& time);
	};

	class SearchBoth : public SearchState {
	protected:
		DTime m_start_time;
	public:
		DTime FindCloserTime(const DTime& reference, const DTime& left, const DTime& right);
	};

	class SearchBothPreferCloser : public SearchBoth {
		DTime m_right_result;

		bool SwitchToGraphThatMayHaveData();
	public:
		virtual void Enter(const DTime& time);
		virtual void HandleLeftResponse(wxDateTime& time);
		virtual void HandleRightResponse(wxDateTime& time);
	};

	class SearchBothPreferLeft : public SearchBoth {
		DTime m_left_result;
	public:
		virtual void Enter(const DTime& time);
		virtual void HandleLeftResponse(wxDateTime& time);
		virtual void HandleRightResponse(wxDateTime& time);
	};

	class SearchBothPreferRight : public SearchBoth {
		DTime m_right_result;
	public:
		virtual void Enter(const DTime& time);
		virtual void HandleLeftResponse(wxDateTime& time);
		virtual void HandleRightResponse(wxDateTime& time);
	};

	State* m_states[FIRST_UNUSED_STATE_ID];

	State* m_state;

	class TimeReference {
		int m_month;
		int m_day;
		wxDateTime::WeekDay m_wday;
		int m_hour;
		int m_minute;
		int m_second;
		int m_milisecond;
	public:
		TimeReference(const wxDateTime &datetime);
		void Update(const DTime& time); 
		DTime Adjust(PeriodType to_period, const DTime& time);
	} m_time_reference;

	ConfigManager *m_config_manager;

	DTime m_current_time;

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

	void FetchData();

	/**Issues queriers for probes in @see m_values table that are is empty state*/
	void TriggerSearch();

	DatabaseQuery* CreateQuery(const std::vector<time_t> &times) const;

	void NoDataFound();

	void MoveToTime(const DTime &time);

	DTime ChooseStartDate(const DTime& found_time, const DTime& suggested_start_time = DTime());

	void DoSet(DrawSet *set);

	void DoSwitchNextDraw(int dir);

	DTime DoSetSelectedDraw(int draw_to_select);

	void ThereIsNoData(const DTime& time);

	DTime SetSelectedDraw(int draw_to_select);

	void EnterState(STATE state, const DTime &time);

public:
	DrawsController(ConfigManager *config_manager, DatabaseManager *database_manager);

	~DrawsController();

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

	void DrawInfoAverageValueCalculationChanged(DrawInfo* di);

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

	std::pair<time_t, time_t> GetStatsInterval();

	static szb_nan_search_condition search_condition;

	enum SORTING_CRITERIA {
		NO_SORT,
		BY_AVERAGE,
		BY_MAX,	
		BY_MIN,
		BY_HOURSUM
	};

	void SortDraws(SORTING_CRITERIA criteria);

	void ParamDataChanged(TParam* param);
};

#endif
