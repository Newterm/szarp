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

#ifndef __TEXECUTE_H__
#define __TEXECUTE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/time.h>

#include "classes.h"
#include "tstatus.h"
#include "texecsect.h"
#include "texecalarm.h"

/** Object of this type is responsible for managing 'execute sections'
 * mechanism in program. */
class TExecute {
	public:
		TExecute(TStatus *status);
		/** Should be called with signals blocked because signal
		 * handlers may manipulate sections list. */
		~TExecute();
		/** Loads config using libpar. Libpar library should be
		 * initialized.
		 * @return 0 on success, 1 on error 
		 */
		int LoadConfig();
		/** Marks sections that should be run on given cycle. */
		void MarkToRun(time_t t);
		/** Runs next sections that should be run on specified time 
		 * cycle. 'current' pointer is used to indicate which
		 * sections should be reviewed.
		 */
		void RunSections();
		/** Handles SIGCHLD signal. Check what childs have exited, 
		 * updates sections and alarms list and calls RunSection().
		 */
		void SigChldHandler(int signum);
		/** Handles SIGALRM signal. Sends terminate or kill signals
		 * if command is to be interrupted, removes expired alarm
		 * from list and calls NextAlarm(). */
		void SigAlrmHandler(int signum);
		/** Preparations for alarm processing. Should be called
		 * once, before first WaitForCycle() call. Sets SIG_ALRM
		 * and SIG_CHLD signal handlers, and sets alarm for the
		 * begining of next 10 minutes cycle. */
		void GetReady();
		/** Process alarm and SIG_CHLD signals in loop, 
		 * exit on new cycle time. Sets alarm for the begining
		 * of next cycle. */
		void WaitForCycle();
		/** Run specified command in shell with output redirected
		 * to stdout_file and stderr_file..
		 * Actually, 'sh -c "<command>"' is executed. 
		 * @param command shell command to execute
		 * @return pid of new process on success, -1 on error
		 */
		int RunCommand(char *command);	
		/** Inserts alarm into list, sorted by exptime. Replaces
		 * currently set alarm if needed.
		 * @param alarm alarm to insert */
		void InsertAlarm(TExecAlarm *alarm);
	protected:
		/**
		 * Checks for validity, creates and adds new section to list. 
		 * NULL values for arguments means default values.
		 * @param timestr string describing time of section's
		 * executing, not NULL
		 * @param commandstr section's command line, not NULL
		 * @param retrystr section's retry string, may be NULL
		 * @param parallelstr section's 'parallel' string, may be
		 * NULL
		 * @param limitstr time limit string, may be NULL
		 */
		int CreateSection(char *timestr, char *commandstr,
				char *retrystr, char *parallelstr,
				char *limitstr);
		/** Loads configuration for sections from szarp.cfg file.
		 * @param section name of section
		 * @return 0 on success, 1 on error (cannot allocate memory)
		 */
		int LoadSectionConfig(char *section);
		/** Set alarm for first element of 'alarms' list. */
		void NextAlarm();
		/**
		 * List of execute sections.
		 */
		TExecSection * sections;
		TExecSection * current;	/**< pointer to last reviewed section,
					  NULL at the begining ot the cycle */
		TExecAlarm *alarms;	/**< list of alarms to process, sorted
					by alarm expiration time */
		char* stdout_file;	/**< path for file used for saving 
					  standard output of executed 
					  commands */
		char* stderr_file;	/**< same as stdout_file, but for 
					  stderr */
		TStatus *status;	/** object for collecting stats */
};

#endif
