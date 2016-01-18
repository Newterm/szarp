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
 * prober - daemon for writing 10-seconds data probes to disk
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 */

#ifndef __TPROBER_H__
#define __TPROBER_H__

#include "szarp_config.h"

#include "twriter.h"
#include "tparcook.h"
#include "tsaveparam.h"
#include "prober.h"
#include "fileremover.h"

/** Application object responsible for saving data to base. */
class TProber : public TWriter {
	public:
		TProber(int probes_buffer_size = 0);
		~TProber();
		int LoadConfig(const char *section);
		int LoadIPK();
		/** Write data to base.
		 * @param force_write force write in the middle of the cycle, used during program termination */
		void WriteParams(bool force_write = false);
		/** Buffered write data to base.
		 * @param force_write force write in the middle of the cycle, used during program termination */
		void WriteParamsBuffered(bool force_write = false);
		/** Wait until begining of next cycle. During wait we also check if we need to remove
		 * outdated cache files.
		 * @param period cycle length in seconds
		 * @param current time (in seconds since EPOC)
		 * @return planned waiting time, for debug purposes
		 */
		time_t WaitForCycle(time_t period, time_t t);
		/** Check if writes are buffered */
		bool IsBuffered();
		/** Get write buffer size */
		int GetBuffSize();
	protected:
		short int * buffer;	/**< buffer for data to save */
		bool all_written;	/**< flag for marking that all available data was saved */
		FileRemover m_fr;	/**< */

		int m_probes_buffer_size; /** < if larger than 0, writes are buffered */
};

#endif
