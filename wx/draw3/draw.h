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

#include <queue>
#include <vector>

/**Simple structore representing numeric interval*/
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

	/**Denotes if there is a remark associated with time this value refers to*/
	bool m_remark;

	/**@return true if this struct holds value received from database and it is not SZB_NODATA*/
	bool IsData() const;

	/**@return true is response for this value has not yet been received or the value of this probe 
	 * is not SZB_NODATA*/
	bool MayHaveData() const;

	ValueInfo();
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

	/**Count of no nodata values withing @see m_stat interval*/
	int m_count;

	/**Number of fetched probes*/
	int m_probes_count;

	/**Number of fetched probes that are not no-data values*/
	int m_data_probes_count;

	/**Data probes percentage*/
	double m_data_probes_ratio;

	size_t m_number_of_values;

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
	void SwitchFilter(int f, int t);

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

	/**Add value to a table.
	 * @param dq value description
	 * @param view_values_changed output parameter, set to true if value withing @see m_view interval has changed*/
	void AddData(DatabaseQuery *q, bool &view_values_changed, bool &stats_updated);

	/**Method that calculates actual value of table entries after reception of response from db.
	 * @param idx index of probe that has been just received from database
	 * @param stats_updated output param, indiactes if value of any entry wihin @see m_stats interval has changed
	 * @param view_updated output param, indiactes if value of any entry wihin @see m_view interval has changed*/
	void UpdateProbesValues(int idx, bool &stats_updated, bool &view_updated);

	void InsertValue(DatabaseQuery::ValueData::V *v, bool &view_values_changed, bool &stats_updated);

	/**@return 'virtual' size of table. That is a size of @see m_view interval*/
	size_t size() const;
	
	/**Resets contents of table. All entries and statistical values are set to no data.*/
	void clean();

	/**@return actual length of table*/
	size_t len() const;

	/**Update statisctis with value of entry located given idx*/
	void UpdateStats(int idx);

	/**Sets number of values the view range holds*/
	void SetNumberOfValues(size_t size);

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


/**Represents displayed draw.
 * This class seems to be too large. shall be split, maybe using state pattern*/
class Draw {
	friend struct ValuesTable;
public:
	typedef ValuesTable VT;
private:
	/**For debugging purposes not used in the code*/
	static void PrintTime(const DTime &time);
	/**Draw number*/
	int m_draw_no;

	/**Parent @see DrawsWidget.*/
	DrawsController* m_draws_controller;

	/**Draw we are acting upon*/
	DrawInfo *m_draw_info;

	/**Indicates if draw is blocked*/ 
	bool m_blocked;

	/**Object handling mapping indices of @see m_values table to correspoding
	 * moments in time and vice versa.*/
	TimeIndex m_index;

	/**Time correspondig to m_start index of m_stats interval*/
	DTime m_stats_start_time;

	/**Probes' values table*/
	VT m_values;

	/**Is set to false if for given param there is no data in database*/
	bool m_param_has_data;

	/**Enabled attribute*/
	bool m_enabled;

	DrawsObservers *m_observers;
public:
	Draw(DrawsController* draws_controller, DrawsObservers *observers, int draw_no);
	/**return current period type*/
	const PeriodType& GetPeriod() const;

	/**Inserts value to to @m_values table and given position.
	 * @param index position of date in @see m_values table
	 * @param value of data
	 * @param sum of all probes composing value*/
	void AddData(DatabaseQuery *q, bool &data_within_view);

	DTime GetStartTime() const;

	/**Return last displayed @see DTime object*/
	DTime GetLastTime() const;

	const TimeIndex& GetTimeIndex();

	/**@return DTime object corresponding to given index in @see m_values table*/
	DTime GetDTimeOfIndex(int i) const;

	/**Calculates index in @see m_values table of give time probe
	 * @param time time to find index for (must be adjusted to period)
	 * @return index value or -1 if the data does not fit 
	 * into currently displayed time range*/
	int GetIndex(const DTime& time, int *dist = NULL) const;

	/**return currently stored probes' values*/
	const VT& GetValuesTable() const;

	DatabaseQuery* GetDataToFetch();

	/**Sets draw info. Param must be in @see state STOP otherwise
	 * assertion is rised. @see m_current_time is set to invalid*/
	void SetDraw(DrawInfo* info);
	
	/**Handles response for datebase. @see HandleDataResponse and @see HandleSearchResponse*/
	void DatabaseResponse(DatabaseQuery *query);

	/* @return false if current param has no data in database, true otherwise*/
	bool GetNoData() const;

	void SetNoData(bool no_data = true); 

	void MoveToTime(const DTime& time);

	void SetPeriod(const DTime& time, size_t number_of_values);

	/**@return @see DrawInfo associtaed with the draw*/
	DrawInfo* GetDrawInfo() const;

	/**@return time given index in @see m_values table corresponds to*/
	DTime GetTimeOfIndex(int i) const;

	/**return true if draw is enabled*/
	bool GetEnable() const;

	/**return true if draw is blocked*/
	bool GetBlocked() const;

	/**sets blocked attribute*/
	void SetBlocked(bool enable);

	/**@return true if param is selected*/
	bool GetSelected() const;

	/**return draw ordinal number*/
	int GetDrawNo() const;

	/**sets double cursor
	 * @return true if double cursor was successfully set*/
	void StartDoubleCursor(int index);

	/**stops double cursor mode*/
	void StopDoubleCursor(); 

	/**Move statistics interval by provided displacement
	 * @param disp statistics displacement*/
	void DragStats(int disp);

	int GetStatsStartIndex();

	/**for debbuging purposes - prints time of index*/
	void PrintTimeOfIndex(int i);

	/**Sets a filter. Level of filter determines how many probes are taken into account 
	 * when calculating fianal displayed value. The displayed value is an average value
	 * of a given probe and 2 * l (where l is a filter level) surrounding probes.
	 * @param filter 'level' of filter */
	void SwitchFilter(int f, int t);

	/**Refetches data from database
	 * @param auto_refresh if set to true only probles all fetched again, if false only those
	 * with no data values*/
	void RefreshData(bool auto_refresh);

	void SetNumberOfValues(size_t values_number);

	void SetRemarksTimes(std::vector<wxDateTime>& times);

	void SetEnable(bool enable);

	int GetCurrentIndex() const;

	bool GetDoubleCursor();

	wxDateTime GetCurrentTime();

	DrawsController* GetDrawsController();
};

#endif
