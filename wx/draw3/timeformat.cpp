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
#include "ids.h"
#include "timeformat.h"

void printstring(const wchar_t *c) {
	FILE *f = fopen("/tmp/draw3.log", "a+");
	fprintf(f, "%ls\n", c);
	fclose(f);
}

void printstring2(const wchar_t *c) {
	fprintf(stderr, "%ls", c);
}

wxString FormatTime(const wxDateTime &time, PeriodType period) {
	wxString ret(_T(""));

	if (!time.IsValid()) {
		return ret;
	}

	if (period == PERIOD_T_SEASON) {
		wxDateTime end = time;
		end += wxTimeSpan::Days(6);
		ret = wxString(_T(" ")) + time.Format(_T("%Y, ")) + _("week") + 
#ifdef MINGW32
			time.Format(_T(" %W, "));
#else
			time.Format(_T(" %V, "));
#endif
		ret += time.Format(_T("%B")) + wxString::Format(_T(" %d - "), time.GetDay());
		if (end.GetMonth() != time.GetMonth())
			ret += end.Format(_T("%B "));
		ret += wxString::Format(_T("%d"), end.GetDay());
	} else if (period == PERIOD_T_WEEK) {
		wxDateTime end = time + wxTimeSpan::Minutes( 7 * 60 + 50);

		ret = time.Format(_T(" %Y, %B, "));
#ifdef MINGW32
		ret += time.Format(_T("%d, "));
		ret += _("week") + time.Format(_T(" %W, %A, "));
		ret += _("hour") + time.Format(_(" %H:%M")) + end.Format(_T("-%H:%M"));
#else
		ret += time.Format(_T("%e, "));
		ret += _("week") + time.Format(_T(" %V, %A, "));
		ret += _("hour") + time.Format(_(" %R")) + end.Format(_T("-%R"));
#endif
	} else {
		int minute = time.GetMinute();
		if (period != PERIOD_T_30MINUTE
				&& period != PERIOD_T_5MINUTE
				&& period != PERIOD_T_MINUTE
				&& period != PERIOD_T_30SEC)
			minute = minute / 10 * 10;

		int second = time.GetSecond();
		if (period != PERIOD_T_5MINUTE && period != PERIOD_T_MINUTE && period != PERIOD_T_30SEC)
			second = second / 10 * 10;
			
		switch (period) {
			case PERIOD_T_30SEC:
			case PERIOD_T_MINUTE:
				ret = wxString::Format(_T(".%01d"), time.GetMillisecond() / 100);
			case PERIOD_T_5MINUTE:
			case PERIOD_T_30MINUTE:
				ret = wxString::Format(_T(":%02d"), second) + ret;
			case PERIOD_T_DAY:
			case PERIOD_T_OTHER :
				ret = wxString(_T(", ")) + _("hour") + time.Format(_T(" %H:")) + 
					wxString::Format(_T("%02d"), minute) + ret;
			case PERIOD_T_MONTH :
				ret = wxString(_T(", ")) 
#ifdef MINGW32
					+ time.Format(_T("%d, ")) 
#else
					+ time.Format(_T("%e, ")) 
#endif
					+ _("week") 
#ifdef MINGW32
					+ time.Format(_T(" %W, %A")) 
#else
					+ time.Format(_T(" %V, %A")) 
#endif
					+ ret;
			case PERIOD_T_YEAR :
				ret = _T(" ") + time.Format(_T("%Y, %B")) + ret;
				break;
			case PERIOD_T_DECADE :
				ret = _T(" ") + time.Format(_T("%Y")) + ret;
				break;
			default:
				break;
		}
		
	}

	return ret;
}
