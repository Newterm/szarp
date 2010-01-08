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
#include "twriter.h"
#include "tparcook.h"
#include "tsaveparam.h"
#include "texecute.h"
#include "tstatus.h"

/** Application object responsible for saving data to base. */
class TMeaner : public TWriter {
	public:
		TMeaner(TStatus *status);
		~TMeaner();
		int LoadConfig(const char *section);
		int LoadIPK();
		/** Initializes 'execute sections' staff. */
		void InitExec();
		/** Write data to base. */
		void WriteParams();
		/** @return pointer to execute object */
		TExecute *GetExec();
	protected:
		TStatus* status;	/**< Object for collecting and saving status
					  parameters. */
		TExecute* executer;	/**< Object for executing jobs. */
		TSaveParam**sparams;	/**< Table of status params. */
		int sparams_len;	/**< Length of status params table. */
};

#endif
