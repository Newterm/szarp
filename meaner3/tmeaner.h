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
 * tmeaner.h
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#ifndef __TMEANER_H__
#define __TMEANER_H__

#include "szarp_config.h"

#include "classes.h"
#include "tparcook.h"
#include "tsaveparam.h"
#include "texecute.h"
#include "tstatus.h"

/** Application object responsible for saving data to base. */
class TMeaner {
	public:
		TMeaner(TStatus *status);
		~TMeaner();
		/** Load configuration for object using libpar.
		 * @return 0 on success, 1 on error */
		int LoadConfig(void);
		/** Check existence of directory for base, write permisions,
		 * and for free space on device.
		 * @return 0 on success, 1 on error */
		int CheckBase();
		/** Load IPK configuration data. 
		 * @return 0 on success, 1 on error */
		int LoadIPK();
		/** Sets signal handlers and signal blocking.
		 * Policy for signal handling is like that:
		 * - 'SIGFPE', 'SIGQUIT', 'SIGILL', 'SIGSEGV' and 'SIGBUS'
		 *   signals are considered 'critical', handler for them
		 *   just writes log message and then restores default
		 *   action and raises signal again - program terminates
		 *   without any cleanup
		 * - 'SIGTERM', 'SIGINT' and 'SIGHUP' - 'termination' signals
		 *   are blocked during signal handling and while reading data 
		 *   from parcook, if we are currently writing data flag is 
		 *   set to mark that we should exit after writing cycle,
		 *   elsewhere we are exiting by reseting default handler and
		 *   raising signal; SA_RESTART flag is used for this signals
		 * - aditional handlers for SIGCHLD and SIGALRM are set for
		 *   'execute sections' mechanism, this is done by methods
		 *   to TExecute class, list of runing childs and sorted list
		 *   of alarms is used; these signals are not blocked while
		 *   reading or writing data.
		 * @return 0 on success, 1 on error */
		int InitSignals();
		/** Initializes communication with parcook. Just calles
		 * TParcook::Init() method.
		 * @return 0 on success, 1 on error */
		int InitReader();
		/** Initializes 'execute sections' staff. */
		void InitExec();
		/** Connects to parcook and reads data from parcook. Program
		 * terminates if connection with parcook failed. */
		void ReadParams();
		/** Write data to base. */
		void WriteParams();
		/** @return pointer to execute object */
		TExecute *GetExec();
	protected:
		TStatus* status;	/**< Object for collecting and saving status
					  parameters. */
		TParcook* parcook;	/**< Object for communicating with 
					  parcook */
		TExecute* executer;	/**< Object for executing jobs. */
		std::wstring ipk_path;	/**< Path to IPK config file */
		std::wstring data_dir; 	/**< Path to directory with base 
					  files */
		TSzarpConfig* config;	/**< IPK configuration data */
		TSaveParam** params;	/**< Table of params. */
		int params_len;		/**< Length of params table. */
		TSaveParam**sparams;	/**< Table of ststus params. */
		int sparams_len;	/**< Length of status params table. */
};

#endif
