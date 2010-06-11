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
 * $Id: wxgraphs.h 1 2009-06-24 15:09:25Z isl $
 */

#ifndef __GRAPHSUTILS_H__
#define __GRAPHSUTILS_H__

struct SeasonLimit {
	size_t index;
	bool summer;				
	int day;	
	int month;
};


std::vector<SeasonLimit> get_season_limits_indexes(DrawsSets *ipk, Draw* draw);

wxString get_date_string(PeriodType period, const wxDateTime& prev_date, const wxDateTime &date);

wxString get_short_day_name(wxDateTime::WeekDay day);


#endif
