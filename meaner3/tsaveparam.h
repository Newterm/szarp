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
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#ifndef __TSAVEPARAM_H__
#define __TSAVEPARAM_H__

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"


#include "tstatus.h"
#include "szarp_config.h"

namespace fs = boost::filesystem;


/** Class represents parameter saved to base. */
class TSaveParam {
	public:
		/** Creates object corresponding to given IPK parameter.
		 * @param p corresponding IPK parameter */
		TSaveParam(TParam* p);
		/** Creates object for param with given name. Used for status
		 * parameters.
		 * @param name name of parameter */
		TSaveParam(const std::wstring& name);
		~TSaveParam();
		/** Write param to base.
		 * @param directory base directory
		 * @param t save time
		 * @param data pointer to buffer with data to write
		 * @param data_count number of elements in buffer to save
		 * @param status if not NULL status object is used for counting
		 * not-null parameters
		 * @param overwrite 0 if is overwriting of existing data 
		 * is not allowed, 1 otherwise
		 * @param force_nodata 1 to force writing no-data values to file
		 * @param length of probe in seconds, default value for meaner, 10 for prober
		 * @return 0 on success, 1 on error
		 */
		int Write(const fs::wpath& directory, 
				time_t t, 
				short int* data, 
				size_t data_count,
				TStatus *status,
				int overwrite, 
				int force_nodata, 
				time_t probe_length);
		/** Write param to base - version that saves one element.
		 * @see Write
		 */
		int Write(const fs::wpath& directory, 
				time_t t, 
				short int data, 
				TStatus *status,
				int overwrite = 0,
				int force_nodata = 0,
				time_t probe_length = SZBASE_PROBE);
		/** Write 10-seconds probe to disk cache.
		 * @see Write
		 */
		int WriteProbes(const fs::wpath& directory, time_t t, short int* data, size_t data_count);
	protected:
		std::wstring cname;	/**< Encoded name of parameter */
};

#endif

