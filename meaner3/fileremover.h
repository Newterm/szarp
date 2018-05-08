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
 * Removing outdated files from 10-seconds probes cache.
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 */

#ifndef __FILEREMOVER_H__
#define __FILEREMOVER_H__

#include <string>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost::filesystem;

/**
 * Deleting outdated cache files.
 */
class FileRemover {
	public:
		/** Initialize cache directory and number of months to keep cache. */
		void Init(const path& dir, int months);

		/** Try to run. Remover is run if it's first time it is run,
		 * or if month changed from last run or if we should continue
		 * from previous run.
		 * @param maxtime time as time_t to stop when reached
		 * @return number of seconds used by method run
		 */
		time_t Go(time_t maxtime);
	protected:
		/** Sets m_current_year and m_current_month from time t. 
		 * @param t current time
		 */
		void TimeToYearMonth(time_t t);
		/** Reset directory iterator. */
		void Reset();

		bool ShouldRemove(const path& p);
		/** Check if file is outdated - older then m_months.
		 * @return true if file is outdated and should be remove, false otherwise
		 */
		bool FileOutdated(const string& filename);
		/** Extracts year and month from filename.
		 * @param filename name of file to check
		 * @param year returned year
		 * @param month returned month
		 * @return true if extraction was ok, false on error
		 */
		bool FilenameToYearMonth(const string& filename, int& year, int& month);

		int m_months;		/**< number of full months to keep */
		path m_dir;		/**< path to cache directory */
		recursive_directory_iterator m_iter;
					/**< directory iterator */
		int m_last_month;	/**< month of last run */
		int m_current_year;	/**< current year */
		int m_current_month;	/**< current month */
};

#endif /* __FILEREMOVER_H__ */

