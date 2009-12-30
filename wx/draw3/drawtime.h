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

#ifndef __DRAWTIME_H__
#define __DRAWTIME_H__


/*"Draw time", object wraping wxDateTime. It's behaviour, namely is addition and substraction operations,
 * are @see m_period sensitive*/
class DTime {
	/**current period*/
	PeriodType m_period;
	/**object holding time*/
	wxDateTime m_time;
	public:
	DTime(const PeriodType &period = PERIOD_T_OTHER, const wxDateTime& time = wxInvalidDateTime);
	/**Rounds time to the current time period, for PERIOD_T_YEAR it is set
	 * to begining of the month, for PERIOD_T_DAY minutes are set to 0
	 * and so on*/
	DTime& AdjustToPeriod();
	/**@return wxDateTime object representing held time*/
	const wxDateTime& GetTime() const;
	/**@return period object*/
	const PeriodType& GetPeriod() const;
	
        /** Adjusts object to current period. For example if
       	  * period is PERIOD_T_YEAR, then date is set to begining of the month.
          * For period PERIOD_T_OTHER this in no op.*/
	DTime& AdjustToPeriodStart();

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
	DTime& SetPeriod(PeriodType pt);

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

	operator const wxDateTime&();
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

	size_t m_number_of_values;

	void UpdatePeriods();

	public:
	TimeIndex(const PeriodType& draw);

	TimeIndex(const PeriodType& draw, size_t max_values);
	/**Gets index of given datetime*/
	int GetIndex(const DTime& time, int *dist = NULL) const;
	/**Gets current start time*/
	const DTime& GetStartTime() const;
	/**@return last time for current time period and set start time*/
	DTime GetLastTime() const;

	DTime GetFirstNotDisplayedTime() const;
	/**@return time represented by given index position*/
	DTime GetTimeOfIndex(int i) const;
	/**Informs object that period has changed*/
	void SetNumberOfValues(size_t values_count);
        /** Adjusts object to current period span. For example if
       	  * period is PERIOD_T_YEAR, then date is set to begining of the year.
          * For period PERIOD_T_OTHER this in no op.*/
	DTime AdjustToPeriodSpan(const DTime& time) const;
	/**Sets current start time*/
	void SetStartTime(const DTime &time);
	/**Sets current start time*/
	void SetStartTime(const DTime &time, size_t number_of_values);
	/**@return time resultion for given period*/
	const wxTimeSpan& GetTimeRes() const;
	/**@return date resultion for given period*/
	const wxDateSpan& GetDateRes() const;
	/**@return time span for given period*/
	const wxTimeSpan& GetTimePeriod() const;
	/**@return date span for given period*/
	const wxDateSpan& GetDatePeriod() const;

	static const size_t default_units_count[PERIOD_T_LAST];

        static const int PeriodMult[PERIOD_T_LAST];
};

#endif
