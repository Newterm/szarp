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

#ifndef __TEXECALARM_H__
#define __TEXECALARM_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "texecsect.h"

/** Event handling for execute mechanism. This is simple data storage class,
 * all attributes are public to allow easier access. Class objects are
 * created by static New<*>Alarm methods, default constructor is used
 * by them internally. */
class TExecAlarm {
	public:
		/** Type for alarm types */
		typedef enum { 
			ALRM_NEWCL, 	/**< start new cycle */
			ALRM_STRM, 	/**< send SIGTERM to process */
			ALRM_SKILL,	/**< send SIGKILL to process */
			ALRM_REM	/**< alarm inactive (removed) */
		} AlarmType;
		/** Creates and returns new alarm with ALRM_NEWCL type with
		 * given setting time and corresponding expire time (end of
		 * cycle).
		 * @param settime set time for alarm
		 * @return new alarm object
		 */
		static TExecAlarm *NewCycleAlarm(time_t settime);
		/** Creates and returns new alarm with ALRM_STRM type to
		 * activate on given time and for given section. */
		static TExecAlarm *NewTermAlarm(time_t exptime, TExecSection *s);
		/** Creates and returns new alarm with ALRM_SKILL type to
		 * activate on given time and for given section. */
		static TExecAlarm *NewKillAlarm(time_t exptime, TExecSection *s);
		AlarmType type;	/**< type of alarm */
		time_t exptime;	/**< expiration time of alarm */
		TExecSection *section;	
				/**< section involved, NULL for ALRM_NEWCL */
		TExecAlarm *next;
				/**< next list element */
		/** 1 if alarm is currently set */
		int isset;
};

#endif
