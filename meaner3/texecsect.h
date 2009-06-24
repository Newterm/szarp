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

#ifndef __TEXECSECT_H__
#define __TEXECSECT_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <stdint.h>

#include "crontime.h"

/** Class represents 'execute sections', defined in szarp.cfg file. */
class TExecSection {
	public:
		TExecSection(char *command, 
				ExecTimeType wday,
				ExecTimeType month,
				ExecTimeType day,
				ExecTimeType hour,
				ExecTimeType min10,
				int retry,
				int parallel,
				int limit);
		~TExecSection();
		/** Append new section at the end of list.
		 * @param sections to append (not NULL) */
		void Append(TExecSection *sections);
		/** @return value of 'pid' attribute */
		pid_t GetPid();
		/** Sets pid attribute to 0 to indicate that sections
		 * is not running currently. May be used only if
		 * pid is different from 0 (section is marked as runing). */
		void ClearPid();
		/** @return value of 'command' attribute */
		char *GetCommand();
		/** @return value of 'fails' attribute */
		int GetFails();
		/** @return next list element, may be NULL for last element */
		TExecSection *GetNext();
		/** Called when attempt to execute section failed.
		 * Set retry if needed (increases 'fails' attribute). */
		void CheckRetry();
		/** Sets if section is to be run in current cycle.
		 * @param torun 1 - run, 0 - do not run */
		void MarkRun(int torun);
		/** @return 1 if sections is marked to run, 0 if not */
		int CheckRun();
		/** Checks if section should be run in given time.
		 * @param wday day of week (0 - 6) (Monday - Sunday)
		 * @param month month (0 - 11)
		 * @param mday day of month (0 - 30)
		 * @param hour hour (0-23)
		 * @param min10 (0-5)
		 * @return 1 if section should be run, 0 if not */
		int CheckTime(int wday, int month, int mday, int hour, int min10);
		/** @return 1 if section is parallel, 0 otherwise */
		int CheckParallel();
		/** Run sections and set alarms for it's limit. */
		void Launch();
		/** Set alarm for finishing command for section that was
		 * launched at given time. Should be called with SIGALRM
		 * SIGCHLD and SIGTERM blocked. */
		void SetAlarm(time_t t);
	protected:
		char* command; 	/**< command to execute */
		ExecTimeType wday, month, day, hour, min10;
				/**< time of execution */
		int retry;	/**< number of retries */
		int parallel;	/**< 1 if section can be run in parallel with 
				  others, 0 otherwise */
		int limit;	/**< time limit (in seconds) for section, value
				  0 means no limit, limit is measured
				  from the begining of cycle (not execution 
				  time) */
		int fails;	/**< number of failed attempts */
		pid_t pid;	/**< pid of running process, 0 if sections is 
				  currently not executed */
		int torun;	/**< 1 if section shuold be run in current cycle */
		TExecSection *next;
				/**< next list element */
};

#endif
