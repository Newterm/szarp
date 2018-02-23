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

#include "draw.h"

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

// interface for testing
class StateController {
public:
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
		SEARCH_LATEST,
		/**Draw waits for data to appear on the "left side" of the cursor. */
		WAIT_DATA_LEFT,
		/**Draw waits for data to appear on the "right side" of the cursor. */
		WAIT_DATA_RIGHT,
		/**Draw waits for data to appear as close as it is possible on any side of the cursor 
		* or m_suggested_time*/
		WAIT_DATA_NEAREST,
		/** Move screen fixing start time */
		MOVE_SCREEN_LEFT,
		MOVE_SCREEN_RIGHT,
		/**Database is searched in both directions from proposed time, left search result
		 * is choosen if data is found on the left side, otherwise time neareset to proposed
		 * time is set*/
		SEARCH_BOTH_PREFER_LEFT,
		/**Database is searched in both directions from proposed time, right search result
		 * is choosen if data is found on the right side, otherwise time neareset to proposed
		 * time is set*/
		SEARCH_BOTH_PREFER_RIGHT,

		FIRST_UNUSED_STATE_ID
	};

	virtual ~StateController() {}

	virtual bool GetDoubleCursor() const = 0;

	virtual const TimeIndex& GetCurrentTimeWindow() const = 0;
	virtual const PeriodType& GetPeriod() const = 0;
	virtual size_t GetNumberOfValues(const PeriodType& period) const = 0;	
	virtual const DTime& GetCurrentTime() const = 0;
	virtual Draw* GetSelectedDraw() const = 0;
	virtual const std::vector<Draw*>& GetDraws() const = 0;

	virtual void EnterState(STATE state, const DTime &time) = 0;
	virtual void MoveToTime(const DTime& time) = 0;
	virtual DTime ChooseStartDate(const DTime& found_time, const DTime& suggested_start_time = DTime()) = 0;

	virtual void NotifyStateChanged(Draw *draw, int pi, int ni, int d) = 0;

	// TODO: add direct interface to database
	virtual void FetchData() = 0;
	virtual void SendQueryToDatabase(DatabaseQuery* query) = 0;
	virtual void ChangeDBObservedParamsRegistration(const std::vector<TParam*> &to_deregister, const std::vector<TParam*> &to_register) = 0;

	// TODO: remove (use regular DTime)
	virtual void UpdateTimeReference(const DTime& time) = 0;

	virtual int GetActiveDrawsCount() const = 0;
	virtual int GetSelectedDrawNo() const = 0;
	virtual int GetCurrentIndex() const = 0;

	virtual void SetCurrentIndex(int i) = 0;
	virtual void SetCurrentTime(const DTime& time) = 0;
	virtual DTime SetSelectedDraw(int draw_to_select) = 0;
	virtual bool SetDrawEnabled(int draw_no, bool enable) = 0;

	virtual void NotifyStateNoData(Draw*) = 0;
	virtual void NotifyStateNoData(DrawsController* = nullptr) = 0;

};


class DrawsController : public DBInquirer, public ConfigObserver, public StateController {

	class State {
	protected:
		StateController *m_c;
	public:
		void SetDrawsController(StateController *c);
		
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

		virtual void NewValuesAdded(Draw *draw, bool non_fixed);

		virtual void Leaving() {};

		virtual ~State() {};

		const int RIGHT = 1;
		const int LEFT = -1;
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

		void NewValuesAdded(Draw *draw, bool non_fixed);

		void Enter(const DTime& time);

		void Leaving();

		const DTime& GetStateTime() const;
	};

	class WaitState : public State {
	protected:
		DTime m_time_to_go;
		virtual void onDataNotOnScreen();
		virtual bool checkIndex(const Draw::VT& values, int i);

	public:
		void Reset();
		void NewDataForSelectedDraw();
		virtual void Enter(const DTime& time);
		virtual void CheckForDataPresence(Draw *draw) = 0;
		const DTime& GetStateTime() const;
		void ParamDataChanged(TParam* param) {}
	};

	class WaitDataLeft : public WaitState {
	protected:
		void onDataNotOnScreen() override;

	public:
		void CheckForDataPresence(Draw *draw);
	};

	class WaitDataRight : public WaitState {
	protected:
		void onDataNotOnScreen() override;

	public:
		void CheckForDataPresence(Draw *draw);
	};

	class WaitDataBoth : public WaitState {
	public:
		void CheckForDataPresence(Draw *draw);
	};

	class SearchState : public State {
	protected:
		size_t m_search_query_id = 0;
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
		virtual void HandleRightResponse(wxDateTime& time);
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

	class SearchLatest : public SearchState {
	public:
		void Enter(const DTime&) override;
		void HandleLeftResponse(wxDateTime& time) override;
	};


	class MoveScreenWindowLeft : public SearchBoth {
	public:
		virtual void Enter(const DTime& time);
		virtual void HandleLeftResponse(wxDateTime& time);
	};

	class MoveScreenWindowRight: public SearchBoth {
	public:
		virtual void Enter(const DTime& time);
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
		
		// hols 10 minutes multiples
		int m_10minute;

		// holds minutes mod 10
		int m_minute;

		int m_10second;
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

	void FetchData() override;

	void SendQueryToDatabase(DatabaseQuery* query) override;

	DatabaseQuery* CreateQuery(const std::vector<time_t> &times) const;

	void NoDataFound();

	void MoveToTime(const DTime &time) override;

	DTime ChooseStartDate(const DTime& found_time, const DTime& suggested_start_time = DTime()) override;

	void DoSet(DrawSet *set);

	void DoSwitchNextDraw(int dir);

	DTime DoSetSelectedDraw(int draw_to_select);

	DTime SetSelectedDraw(int draw_to_select) override;

	void EnterState(StateController::STATE state, const DTime &time) override;

public:
	DrawsController(ConfigManager *config_manager, DatabaseManager *database_manager);

	~DrawsController();

	void HandleSearchResponse(DatabaseQuery *query);

	void HandleDataResponse(DatabaseQuery *query);

	std::vector<TParam*> GetSubscribedParams() const;

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


	bool SetDrawEnabled(int draw_no, bool enable) override;

	bool GetDrawEnabled(int draw_no);


	bool SetDoubleCursor(bool double_cursor);

	void SetFollowLatestData(bool follow_latest_data);

	bool GetFollowLatestData();

	bool GetDoubleCursor() const override;

	const std::vector<Draw*>& GetDraws() const override { return m_draws; }

	Draw* GetDraw(size_t draw_no);

	size_t GetDrawsCount();


	void RefreshData(bool auto_refresh);

	void DrawInfoAverageValueCalculationChanged(DrawInfo* di);

	void ClearCache();

	void SendQueryForEachPrefix(DatabaseQuery::QueryType qt);

	void NotifyStateNoData(Draw *draw) override { m_observers.NotifyNoData(draw); }
	void NotifyStateNoData(DrawsController *draws_controller = nullptr) override { draws_controller == nullptr ? m_observers.NotifyNoData(this) : m_observers.NotifyNoData(draws_controller); }

	/**@return current param, queries for values of this param will have higher priority than queries to other params*/
	virtual DrawInfo* GetCurrentDrawInfo();

	/**@return current time, used for queries prioritization*/
	virtual const DTime& GetCurrentTime() const override;

	const TimeIndex& GetCurrentTimeWindow() const override;

	virtual void ConfigurationWasReloaded(wxString prefix);

	void SetCurrentIndex(int i) override { m_current_index = i; }

	void SetCurrentTime(const DTime& time) override;

	virtual void SetModified(wxString prefix, wxString name, DrawSet *set);

	virtual void SetRemoved(wxString prefix, wxString name);

	virtual void SetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set);

	void NotifyStateChanged(Draw *draw, int pi, int ni, int d) override { m_observers.NotifyCurrentProbeChanged(draw, pi, ni, d); }

	void ChangeDBObservedParamsRegistration(const std::vector<TParam*> &to_deregister, const std::vector<TParam*> &to_register) override {
		ChangeObservedParamsRegistration(to_deregister, to_register);
	}

	int GetCurrentIndex() const override;

	int GetActiveDrawsCount() const override { return m_active_draws_count; };

	int GetSelectedDrawNo() const override { return m_selected_draw; }

	/**method is called when response from db is received
	 * @param query object holding response from database, the same instance that was sent to the database via 
	 * @see QueryDatabase method but with proper fields filled*/
	virtual void DatabaseResponse(DatabaseQuery *query);

	Draw* GetSelectedDraw() const override;

	DrawSet *GetSet();


	const PeriodType& GetPeriod() const override;


	void AttachObserver(DrawObserver *observer);

	void DetachObserver(DrawObserver *observer);


	size_t GetNumberOfValues(const PeriodType& period) const override;	

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

	void UpdateTimeReference(const DTime& time) override { m_time_reference.Update(time); }
};

#endif
