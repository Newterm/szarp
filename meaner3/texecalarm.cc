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
 * meaner3 - daemon writing data to base in SzarpBase format
 * SZARP
 * bodas staff
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#include "texecalarm.h"

#include "meaner3.h"
#include "texecsect.h"

TExecAlarm *TExecAlarm::NewCycleAlarm(time_t settime)
{
	TExecAlarm *alarm;

	alarm = new TExecAlarm();
	assert(alarm != NULL);
	/* count time for next cycle */
	alarm->exptime = settime - (settime % BASE_PERIOD) + BASE_PERIOD;
	alarm->type = ALRM_NEWCL;
	alarm->section = NULL;
	alarm->next = NULL;
	alarm->isset = 0;

	return alarm;
}

TExecAlarm *TExecAlarm::NewTermAlarm(time_t exptime, TExecSection *s)
{
	TExecAlarm *alarm;

	alarm = new TExecAlarm();
	assert(alarm != NULL);
	alarm->exptime = exptime;
	alarm->type = ALRM_STRM;
	alarm->section = s;
	alarm->next = NULL;
	alarm->isset = 0;

	return alarm;
}

TExecAlarm *TExecAlarm::NewKillAlarm(time_t exptime, TExecSection *s)
{
	TExecAlarm *alarm;

	alarm = new TExecAlarm();
	assert(alarm != NULL);
	alarm->exptime = exptime;
	alarm->type = ALRM_SKILL;
	alarm->section = s;
	alarm->next = NULL;
	alarm->isset = 0;

	return alarm;
}

