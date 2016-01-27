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
 * parcook.h - communication with parcook
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#ifndef __TPARCOOK_H__
#define __TPARCOOK_H__

#include "szbase/szbfile.h"

typedef enum {
	min10 = 0,
	sec10,
	ProbesType_last
} ProbesType;

class TParcook {
	public:
		TParcook(ProbesType type = min10);
		~TParcook();
		/** Load configuration for object using libpar.
		 * * @return 0 on success, 1 on error */
		int LoadConfig();
		/** Initializes communication with parcook process. 
		 * @param probes_count length of probes table
		 * @return 0 on success, 1 on error */
		int Init(int probes_count);
		/** Init communication with parcook process - buffer memory */
		int InitBuffer();
		/** Gets parameters values from parcook segment. Enters
		 * parcook semaphore, attaches segment, copies values
		 * to internal objects buffer, detaches segment and releases
		 * semaphores. Signals are blocked during execution. On error
		 * method calls termination handler.
		 */
		void GetValues();
		void GetValuesBuffer();
		/** Return probe value for given param from internal object
		 * buffer.
		 * @param i parameter index
		 * @return parameter's probe value
		 */
		short int GetData(int i);
		/** Return buffer size
		 * buffer.
		 * @param i parameter index
		 * @return parameter's buffer size
		 */
		short int GetData(int i, short int* buffer) ;
		short int GetDataPos();
		short int GetDataCount();

	protected:
		ProbesType probes_type;	/**< type of parcook segment we read from */
		char* parcook_path;	/**< path used to obtain semaphore and
					  shared memory identifiers */
		
		int shm_desc;		/**< shared memory descriptor */
		int sem_desc;		/**< semaphore descr */
		int shm_desc_buff;	/**< shared memory descriptor buffer*/
		int sem_desc_buff;	/**< semaphore descr buffer*/
		int probes_count;	/**< length of probes table */
		int buffer_count;	/**< length of buffer */

		SZB_FILE_TYPE* copied;	/**< buffer for copied probes values */
		SZB_FILE_TYPE* buffer_copied;	/**< buffer for copied buffer probes values */
};

#endif

