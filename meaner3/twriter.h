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

#ifndef __TWRITER_H__
#define __TWRITER_H__

#include "szarp_config.h"

#include "tparcook.h"
#include "tsaveparam.h"
#include "texecute.h"
#include "tstatus.h"

/** Base class for TMeaner and TProber - applications for periodically saving data to disk. */
class TWriter {
	public:
		/**
		 * @param type type of parcook segment to read data from
		 */
		TWriter(ProbesType type);
		~TWriter();
		/** Load configuration for object using libpar.
		 * @param section name of cofiguration section in szarp.cfg file
		 * @param datadir_param name of szarp.cfg parameter containing path
		 * to data directory; for safety reason this parameter has different
		 * name for meaner and prober
		 * @return 0 on success, 1 on error */
		int LoadConfig(const char *section, const char* datadir_param = "datadir");
		/** Check existence of directory for base, write permisions,
		 * and for free space on device.
		 * @param req_free_space minimal amount of free space available at
		 * start of program, in bytes; default to 512 KB
		 * @return 0 on success, 1 on error */
		int CheckBase(unsigned long req_free_space = 524288L);
		/** Load IPK configuration data. 
		 * @return number of params to write on success, -1 on error */
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
		 * @param critical_handler handler for critical signals
		 * @param term_handler handler for 'termination' signals
		 * @return 0 on success, 1 on error */
		int InitSignals(void (*critical_handler)(int), void (*term_handler)(int));
		/** Initializes communication with parcook. Just calles
		 * TParcook::Init() method.
		 * @return 0 on success, 1 on error */
		int InitReader();
		/** Connects to parcook and reads data from parcook. Program
		 * terminates if connection with parcook failed. */
		void ReadParams();
		/** Write data to base. */
		void WriteParams() { };
	protected:
		TParcook* parcook;	/**< Object for communicating with 
					  parcook */
		std::wstring ipk_path;	/**< Path to IPK config file */
		std::wstring data_dir; 	/**< Path to directory with base 
					  files */
		TSzarpConfig* config;	/**< IPK configuration data */
		TSaveParam** params;	/**< Table of params. */
		int params_len;		/**< Length of params table. */
};

#endif
