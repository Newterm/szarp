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
#ifndef __DRAW_H__
#define __DRAW_H__
/*
 * draw3 program
 * SZARP
 
 
 * $Id$
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/dcmemory.h>
#include <wx/datetime.h>
#endif

#include <tr1/unordered_map>
#include <queue>
#include <vector>

#include "drawswdg.h"
#include "ids.h"

class InfoWidget;
class DrawObserver;
class DrawInfo;
class Draw;
struct TimeReference;

/**Simple structore representing numberic interval*/
struct Interval {
	/**interval start*/
	int m_start;
	/**interval end */
	int m_end;
		
	/**@return minimum of m_start and m_end values*/
	int Start() const;
	/**@return maximum of m_start and m_end values*/
	int End() const;
	/**moves intervals i.e. adds value of 
	 * param dr to m_start and m_end members*/
	void Move(int dr);
	/**Adds given value to a m_start memeber*/
	void Drag(int d);
	Interval() : m_start(-1), m_end(-1) {}
	/**Caclulates union of intervals*/
	Interval Union(const Interval& interval) const;		
};

/**Represents displayed value*/
struct ValueInfo {

	enum {
		/**This struct does not hold valid value*/
		EMPTY,
		/**Query for this value has been issued but response has not yet been received*/
		PENDING,
		/**Response for this value has been received*/
		PRESENT,
		/**Waiting for neighbouring values to be received*/
		NEIGHBOUR_WAIT
	} state;
	
	/**Value of data*/
	double val;

	/**Sum probes' values*/
	double sum;

	/**Value as received from database*/
	double db_val;

	/**Number of neighbours that also has already received data.*/
	int n_count;

	/**Maximum number of probes composing this value*/
	int max_probes;

	/**Actual number of probes composing this value*/
	int count_probes;

	/**@return true if this struct holds value received from database and it is not SZB_NODATA*/
	bool IsData() const;

	/**@return true is response for this value has not yet been received or the value of this probe 
	 * is not SZB_NODATA*/
	bool MayHaveData() const;

	ValueInfo();
};

/*"Draw time", object wraping wxDateTime. It's behaviour, namely is addition and substraction operations,
 * are @see m_period sensitive*/
class DTime {
	/**current period, shall be refrence to Draw object*/
	const PeriodType& m_period;
	/**object holding time*/
	wxDateTime m_time;
	public:
	DTime(const PeriodType &period, const wxDateTime& time = wxInvalidDateTime);
	/**Rounds time to the current time period, for PERIOD_T_YEAR it is set
	 * to begining of the month, for PERIOD_T_DAY minutes are set to 0
	 * and so on*/
	void AdjustToPeriod();
        /** Adjusts object to current period. For example if
       	  * period is PERIOD_T_YEAR, then date is set to begining of the month.
          * For period PERIOD_T_OTHER this in no op.*/
	void AdjustToPeriodStart();
        /** Adjusts object to current period span. For example if
       	  * period is PERIOD_T_YEAR, then date is set to begining of the year.
          * For period PERIOD_T_OTHER this in no op.*/
	void AdjustToPeriodSpan();
	/**@return wxDateTime object representing held time*/
	const wxDateTime& GetTime() const;
	/**@return period object*/
	const PeriodType& GetPeriod() const;

	DTime& operator=(const DTime&);
	/**Adds specified wxTimeSpan.
	 * @return result of addition*/
	DTime operator+(const wxTimeSpan&) const;
	/**Adds specified wxDateSpan.
	 * @return result of addition*/
	DTime operator+(const wxDateSpan&) const;
	/**Substracts specified wxTimeSpan.
	 * @return result of substraction*/
	DTime operator-(const wxTimeSpan&) const;
	/**Substracts specified wxDateSpan.
	 * @return result of substraction*/
	DTime operator-(const wxDateSpan&) const;

	/**@return true if underlaying time object is valid*/
	bool IsValid() const;

	/**Sets type of perid*/
	void SetPeriod(PeriodType pt);

	/*Starndard comparison operators*/
	bool operator==(const DTime&) const;
	bool operator!=(const DTime&) const;
	bool operator<(const DTime&) const;
	bool operator>(const DTime&) const;
	bool operator<=(const DTime&) const;
	bool operator>=(const DTime&) const;

	bool IsBetween(const DTime& t1, const DTime& t2) const;

	/**@return distance in current period 'units' between dates. For example for period Year
	 * and dates 1st september nad 1st december to result will be 2.*/
	int GetDistance(const DTime& t) const;

	/**Formats date
	 * @return string represeting formatted date*/
	wxString Format(const wxChar* format = wxDefaultDateTimeFormat) const;

};


struct ValuesTable {

	/**Denotes where in @see m_values table are localted probes that
	 * are currently displayed*/
	Interval m_view;
	/**Denotes where in @see m_values table are localted probes that
	 * are used for statistics calculation*/
	Interval m_stats;
		
	/**'parent' @see Draw object*/
	Draw* m_draw;

	/**Table holding probes' values*/
	std::deque<ValueInfo> m_values;

	/**Denotes if we are working in 'double cursor mode (affects table range calculation)*/
	bool m_double_cursor;

	/**Filter level*/
	int m_filter;

	/**Count of no nodata values withing @see m_stat interval*/
	int m_count;

	/**Number of fetched probes*/
	int m_probes_count;

	/**Number of fetched probes that are not no-data values*/
	int m_data_probes_count;

	/**Data probes percentage*/
	double m_data_probes_ratio;

	/**Statistics values*/
	double m_sum;
	double m_hsum;
	double m_min;
	double m_max;

	/**Starts double cursor mode*/
	void EnableDoubleCursor(int i);
	/**Stops double cursor mode*/
	void DisableDoubleCursor();
	/**Caclulate probe value*/
	void CalculateProbeValue(int index);

	/**Sets filter, recalculate values*/
	void SetFilter(int f);

	/**Standard table access operators. Note: actual
	 * value returned from @see m_values table is located
	 * at i(parameter) + m_view.Start()*/
	ValueInfo& at(size_t i) ;

	const ValueInfo& at(size_t i) const;

	ValueInfo& operator[](size_t i) ;

	const ValueInfo& operator[](size_t i) const ;

	/**"Raw table access operators*/
	ValueInfo& Get(size_t i);

	const ValueInfo& Get(size_t i) const;

	/** Recalculates statical values*/
	void RecalculateStats();

	/** Clear 'statistical' values*/
	void ClearStats();

	/** Moves table by given interval.
	 * @param in interval that denotos where, relatively to it's current position,
	 * table should be located*/
	void Move(const Interval &in);

	/** Displaces @see m_view interval by given number of units.
	 * If table is not in double cursor mode also @see m_stats intervals is moved*/
	void MoveView(int dr);

	/** Displaces @see m_stats interval by given number of units. Recalculates values*/
	void DragStats(int dr);

	/**Notify all observers about change of statistic values*/
	void NotifyStatsChange();

	/**Add value to a table.
	 * @param dq value description
	 * @param view_values_changed output parameter, set to true if value withing @see m_view interval has changed*/
	void AddData(DatabaseQuery* dq, bool &view_values_changed);

	/**Method that calculates actual value of table entries after reception of response from db.
	 * @param idx index of probe that has been just received from database
	 * @param stats_updated output param, indiactes if value of any entry wihin @see m_stats interval has changed
	 * @param view_updated output param, indiactes if value of any entry wihin @see m_view interval has changed*/
	void DataInserted(int idx, bool &stats_updated, bool &view_updated);

	void InsertValue(DatabaseQuery::ValueData::V &v, bool &view_values_changed, bool &stats_updated);

	/**Adjust table size to given period*/
	void SetPeriod(PeriodType pt);
	
	/**@return 'virtual' size of table. That is a size of @see m_view interval*/
	size_t size() const;
	
	/**Resets contents of table. All entries and statistical values are set to no data.*/
	void clean();

	/**@return actual length of table*/
	size_t len() const;

	/**Update statisctis with value of entry located given idx*/
	void UpdateStats(int idx);

	/**Pushes empty value on the front of the table.*/
	void PushFront();

	/**Pops a value from the front of the table*/
	void PopFront();

	/**Pushes new empty entry to the end of the table*/
	void PushBack();

	/**Pops entry from the end of the table*/
	void PopBack();

	void ResetValue(int i);

	ValuesTable(Draw* draw);

};

/**Maps @see DTime objects to corresponding indexes into @see m_values table*/
class TimeIndex {
	/**current start time*/
	DTime m_time;

        wxDateSpan m_dateres;
      		wxTimeSpan m_timeres;	/**< dateres and timeres represents
                                  	resolution of time axis - minimal
                                difference between time meseaured.
                                dateres is responsible for date part
                                (more then day). Usually only one of the 
				values is used. */
	wxDateSpan m_dateperiod;         
	wxTimeSpan m_timeperiod; 
				/**< dateperiod and timeperiod represents
				period of time on time axis. */

	public:
	TimeIndex(const PeriodType& draw, size_t max_values);
	/**Gets index of given datetime*/
	int GetIndex(const DTime& time, size_t max_idx, int *dist = NULL) const;
	/**Gets current start time*/
	const DTime& GetStartTime() const;
	/**@return last time for current time period and set start time*/
	DTime GetLastTime(size_t values_count) const;
	/**@return time represented by given index position*/
	DTime GetTimeOfIndex(int i) const;
	/**Informs object that period has changed*/
	void PeriodChanged(size_t values_count);
	/**Sets current start time*/
	void SetStartTime(const DTime &time);
	/**@return time resultion for given period*/
	const wxTimeSpan& GetTimeRes() const;
	/**@return date resultion for given period*/
	const wxDateSpan& GetDateRes() const;
	/**@return time span for given period*/
	const wxTimeSpan& GetTimePeriod() const;
	/**@return date span for given period*/
	const wxDateSpan& GetDatePeriod() const;

};



/**Represents displayed draw.
 * This class seems to be too large. shall be split, maybe using state pattern*/
class Draw {
	friend struct ValuesTable;
public:
	typedef ValuesTable VT;
private:
	/**For debugging purposes not used in the code*/
	static void PrintTime(const DTime &time);
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

	/**Draw number*/
	int m_draw_no;

	/**Parent @see DrawsWidget.*/
	DrawsWidget* m_draws_widget;

	/**Draw we are acting upon*/
	DrawInfo *m_draw_info;

	/**Current period type*/
	PeriodType m_period;

	/**Indicates if draw is selected*/
	bool m_selected;

	/**Indicates if draw is blocked*/ 
	bool m_blocked;

	/**Object handling mapping indices of @see m_values table to correspoding
	 * moments in time and vice versa.*/
	TimeIndex m_index;
	/**Time we would like to display. Start time of each search search queries*/
	DTime m_proposed_time;
	/**Upon leave of @see SEARCH_BOTH state attempt to set this time as @see m_start_time*/
	DTime m_suggested_start_time;
	/**Current (i.e. time where cursor is located) time*/
	DTime m_current_time;
	/**Time correspondig to m_start index of m_stats interval*/
	DTime m_stats_start_time;

	/**Result of search for search for data in left direction*/
	DTime m_left_search_result;
	/**Result of search for search for data in right direction*/
	DTime m_right_search_result;
	/**Probes' values table*/
	VT m_values;

	/**Indicates if search for data in left direction has been received*/
	bool m_got_left_search_response;
	/**Indicates if search for data in right direction has been received*/
	bool m_got_right_search_response;

	/**Is set to false if for given param there is no data in database*/
	bool m_param_has_data;

	/**@see TimeReference */
	TimeReference* m_reference;

	/**Enters given state
	 * @param state - state to enter*/
	void EnterState(STATE state);

	/**Sets @see m_start_time so that time will be located at centre of the 'screen'
	 * @param time to center screen on*/
	void AdjustStartTime(const DTime &time);

	/**Sets @see m_start_time, issues data fetch if necessary
	 * @param time to set*/
	void SetStartTime(const DTime& time);

	/**@return true if data is located with current displayed time span*/
	bool DataFitsCurrentInterval(const DTime& time);

	/**Issues queries for all empty values in @see m_values table*/
	void TriggerDataFetch();

	/**Updates time reference with time given as arguement
	 * @param time time to sync reference with*/
	void UpdateTimeReference(const DTime& time);

	/**Adjust given time to time reference
	 * @param dt time to sync with reference
	 * @return resultant time*/
	DTime SyncWithTimeReference(const DTime &dt);

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

	/**Inserts recived value into @see m_values table.
	 * May cause state transition if param is in one of WAIT* states.*/
	void HandleDataResponse(DatabaseQuery *query);

	/**Inserts value to to @m_values table and given position.
	 * Notifies @see DrawsWidgets about any redraws of DC.
	 * @param index position of date in @see m_values table
	 * @param value of data
	 * @param sum of all probes composing value*/
	void AddData(int i, double value, double sum);

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
	void CheckData();

	/**Sends a search query to the db
	 * @param start - search start time
	 * @param end - search end time
	 * @param direction - direcrion of search*/
	void SendSearchQuery(const wxDateTime& start, const wxDateTime& end, int direction);
	
	/**Sends a cleaning query to the db
	 * @param start -valid data start time
	 * @param end - valid data end time*/
	void SendCleanQuery(const wxDateTime& start, const wxDateTime& end);

	DatabaseQuery* CreateQuery(const std::vector<time_t> &times) const;

	/**Sends a search query to the db*/
	void SendDataQuery(const std::vector<time_t>& time);

	/**Issues queriers for probes in @see m_values table that are is empty state*/
	void TriggerSearch();

	/**If neccesary adjusts @see m_start_time so that provided time will 
	 * be located within displayed within displayed time range. 
	 * @return true if @see m_start_time was modified.*/
	bool GoToTime(const DTime& time);

	/**Starts selected draw activity. @see m_suggested_time must be valid
	 * when this method is called. If @see m_suggested_time does not fit into 
	 * currently displayed interval @see Draw enters @see SEARCH_BOTH state. 
	 * If data for @see m_suggested_time is present and is not 
	 * SZB_NODATA param enters @see DISPLAY state. Otherwise param enters
	 * @see WAIT_DATA_BOTH state.
	 * Updates @see InfoWidget*/
	void StartSelectedDraw();

	/**Observers queue for this draw*/
	std::vector<DrawObserver*> m_observers;

	/**Enabled attribute*/
	bool m_enabled;

	/**Split cursor mode*/
	bool m_split_cursor;

	/**current index in @see m_values table. Note - this variable is assigned
	 * to in @see SetCurrentDispTime and not used for any other purposes*/
	int m_current_index;

	/**Calculates index in @see m_values table of give time probe
	 * @param time time to find index for (must be adjusted to period)
	 * @return index value or -1 if the data does not fit 
	 * into currently displayed time range*/
	int GetIndex(const DTime& time, int *dist = NULL) const;

	/**Return last displayed @see DTime object*/
	DTime GetLastTime() const;

	/**@return DTime object corresponding to given index in @see m_values table*/
	DTime GetDTimeOfIndex(int i) const;

public:
	Draw(DrawsWidget *widget, TimeReference* reference, int draw_no);
	/**return current period type*/
	const PeriodType& GetPeriod() const;

	/**return currently stored probes' values*/
	const VT& GetValuesTable() const;

	/**return currently displayed time*/
	const wxDateTime& GetDisplayedTime() const;

	/**Ataches observer to list of observers
	 * @param observer Observer to attach*/
	void AttachObserver(DrawObserver *observer);

	/**Removes observer from observers' list*/
	void DetachObserver(DrawObserver *draw);

	/**@return 'current' index in @m_values table*/
	int GetCurrentIndex() const;

	/**@return @see m_proposed_time*/
	const wxDateTime& GetCurrentTime() const;

	/**Sets @see m_proposed_time to param value. Draw has to in STOP state
	 * @param*/
	void Select(const wxDateTime &time);

	/**Deselects draw. Cursor is cleared. @see m_current_time is set as invalid.
	 * In any state but @see STOP this method forces transtion of @see Draw
	 * to @see DISPLAY state*/
	void Deselect();

	/**Sets current displayed time. Param value must be adjusted to currenty
	 * displayed time interval and data must be present for this time*/
	void SetCurrentTime(const wxDateTime &time);

	/**Sets draw info. Param must be in @see state STOP otherwise
	 * assertion is rised. @see m_current_time is set to invalid*/
	void SetDraw(DrawInfo* info);
	
	/**Sets period type*/
	void SetPeriod(PeriodType period, const wxDateTime &current_time);

	/**Handles response for datebase. @see HandleDataResponse and @see HandleSearchResponse*/
	void DatabaseResponse(DatabaseQuery *query);

	/* @return false if current param has no data in database, true otherwise*/
	bool HasNoData() const;

	/**If param is not select sets param value as start time*/
	void MoveToTime(const wxDateTime& time);

	/**Starts draw activity*/
	void Start();

	/**Forces transition to @see STOP time*/
	void Stop();

	/**Moves cursor right*/
	void MoveCursorRight(int n = 1);

	/**Moves cursor left*/
	void MoveCursorLeft(int n = 1);

	/**@return @see DrawInfo associtaed with the draw*/
	DrawInfo* GetDrawInfo() const;

	/**Sets @see m_current_dispay time, moves cursor, updates @see InfoWidget.
	 * Notifies @see DrawsWidgets about any redraws of DC.
	 * Shall only be called when param is seleced
	 * @param time time to be set as current time*/ 
	void SetCurrentDispTime(const DTime& time);

	/** Just start this program and experiment with PageUp key and you'll know what this method dooes :)*/
	void MoveScreenRight();

	/** Just start this program and experiment with PageDown key and you'll know what this method dooes :)*/
	void MoveScreenLeft();

	/**Move cursor to first available data in the @see m_values table*/
	void MoveCursorBegin();

	/**Move cursor to last available data in the @see m_values table*/
	void MoveCursorEnd();

	/**@return time given index in @see m_values table corresponds to*/
	wxDateTime GetTimeOfIndex(int i) const;

	/**return true if draw is enabled*/
	bool GetEnable() const;

	/**return true if draw is blocked*/
	bool GetBlocked() const;

	/**sets enabled attribute*/
	void SetEnable(bool enable);

	/**sets blocked attribute*/
	void SetBlocked(bool enable);

	/**@return true if param is selected*/
	bool GetSelected() const;

	/**return draw ordinal number*/
	int GetDrawNo() const;

	/**sets double cursor
	 * @return true if double cursor was successfully set*/
	bool StartDoubleCursor();

	/**@return true if object is in double cursor mode*/
	bool IsDoubleCursor();

	/**stops double cursor mode*/
	void StopDoubleCursor(); 

	/**Starts double cursor mode, at index given as argument, via this
	 * method not selected draw is informed about double cursor being turned on.*/
	void DoubleCursorSet(int idx);

	/**Move statistics interval by provided displacement
	 * @param disp statistics displacement*/
	void DragStats(int disp);

	/**Number of probes composing one 'stripe'. @see ValuesTable may be shifted only by multiplicity
	 * of these value*/
        static const int PeriodMult[PERIOD_T_LAST];
	
	/**for debbuging purposes - prints time of index*/
	void PrintTimeOfIndex(int i);

	/**Sets a filter. Level of filter determines how many probes are taken into account 
	 * when calculating fianal displayed value. The displayed value is an average value
	 * of a given probe and 2 * l (where l is a filter level) surrounding probes.
	 * @param filter 'level' of filter */
	void SetFilter(int level);

	/**Refetches data from database
	 * @param auto_refresh if set to true only probles all fetched again, if false only those
	 * with no data values*/
	void RefreshData(bool auto_refresh);

	void DoubleCursorStopped();

	bool IsTheNewestValue();
};

#endif
