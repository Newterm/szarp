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
#ifndef __TIMEFORMAT_H__
#define __TIMEFORMAT_H__

#include <wx/datetime.h>

/**Returns string representing provided time. Only part
 * of date and time significant for given period is enclosed
 * in resulting string.
 * @param time time to format
 * @param period period for which the string shall be formatted
 * @return string represeting time*/
wxString FormatTime(const wxDateTime &time, PeriodType period);

#endif
