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
 * status parameter
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#ifndef __TSTATUS_H__
#define __TSTATUS_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"



#include "meaner3.h"

namespace fs = boost::filesystem;

/** Object of this class is responsible for collecting and saving special
 * 'status' parameters. These parameters' values are various statistics about
 * program execution. */
class TStatus {
	public:
		/** Available parameters - @see TStatus_ParamNames. */
		typedef enum { 
			PT_RUN,
			PT_PARAMS,
			PT_DEVPS,
			PT_DEFPS,
			PT_DDPS,
			PT_BASEPS,
			PT_NNPS,
			PT_OKPS,
			PT_ERRPS,
			PTE_SECS,
			PTE_TRSECS,
			PTE_RSECS,
			PTE_OKSECS,
			PTE_ERRSECS,
			PTE_INTSECS,
			PT_SIZE /* must be last */
		} ParamType;
		TStatus();
		/** Set value for given parameter to 0.
		 * @param pt type of parameter */
		void Reset(ParamType pt);
		/** Set values of all parameters to 0. */
		void ResetAll();
		/** Sets values of all parameters but PT_RUN to 0.
		* PT_RUN is set to 0 if it is first call of the function
		* to 1 otherwise */
		void NextCycle();
		/** Set value for given parameter.
		 * @param pt type of parameter
		 * @param val new value */
		void Set(ParamType pt, SZB_FILE_TYPE val);
		/** Increase value for given parameter by 1.
		 * @param pt type of parameter 
		 * @return new value */
		SZB_FILE_TYPE Incr(ParamType pt);
		/** Decrease value for given parameter by 1.
		 * @param pt type of parameter 
		 * @return new value */
		SZB_FILE_TYPE Decr(ParamType pt);
		/** @return number of parametrs */
		int GetCount();
		/** @return pointer to name of given parameter */
		const wchar_t *GetName(ParamType pt);
		/** Writes status parameter to base.
		 * Signals should be blocked.
		 * @param pt parameter to write
		 * @param t time of cycle
		 * @param sp corresponding TSaveParam object
		 * @param datadir argument to pass to TSaveParam->Write() method
		 * @return 0 on success, 1 on error */
		int WriteParam(ParamType pt, time_t t, TSaveParam *sp,
				const fs::wpath& data_dir);
	protected:
		/** Array of values for parameters. */
		SZB_FILE_TYPE vals[PT_SIZE];
		/** True unitl @see NextCycle is called.
		 * Denotes if PT_RUN should be set to 0*/
		bool firstTime;
};

/** Array of status parameters names. */
extern const wchar_t * TStatus_ParamNames[TStatus::PT_SIZE];
/** Array of shifts (measured in cycles) for status parameters save time. 
 * If shift is 0, parameter is save normally. If shift is -1, parameter is
 * saved with time of previous cycle. */
extern int TStatus_SaveShift[TStatus::PT_SIZE];
/** Array of values, 1 per parameter. 1 means that coresponding parameter should be
 * reset on each cycle, 0 otherwise. */
extern int TStatus_ResetParam[TStatus::PT_SIZE];


#endif
