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

#include "config.h"
#include "fileremover.h"
#include "liblog.h"
#include <vector>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;

void FileRemover::Init(const path& dir, int months)
{
	m_dir = dir;
	m_months = months;
	Reset();
}

time_t FileRemover::Go(time_t maxtime)
{
	time_t t = time(NULL);
	time_t first = t;
	TimeToYearMonth(t);

	if (m_iter == recursive_directory_iterator()) {
		if (m_last_month == m_current_month) {
			return 0;
		} else {
			Reset();
		}
	}
	sz_log(2, "(Re)starting remover loop, searching for files more then %d months old",
			m_months);
	m_last_month = m_current_month;

	std::vector<path> files2remove;
	for (; m_iter != recursive_directory_iterator(); ++m_iter) {
		if (ShouldRemove(m_iter->path())) {
			files2remove.push_back(m_iter->path());
		}
		t = time(NULL);
		if (t >= maxtime) {
			sz_log(1, "Interrupting remover loop after finding %d files to remove", (int)files2remove.size());
			break;
		}
	}
	for (const auto& p : files2remove) {
		remove(p);
	}
	sz_log(2, "Finished remover loop after removing %d files", (int)files2remove.size());
	return first - t;
}

void FileRemover::TimeToYearMonth(time_t t)
{
	struct tm tm;
	gmtime_r(&t, &tm);
	m_current_year = tm.tm_year + 1900;
	m_current_month = tm.tm_mon + 1;
}

void FileRemover::Reset()
{
	m_iter = recursive_directory_iterator(m_dir);
}

bool FileRemover::ShouldRemove(const path& p)
{
	if (not is_regular_file(m_iter->path())) {
		return false;
	}
#if BOOST_FILESYSTEM_VERSION == 3
	if (FileOutdated(m_iter->path().filename().string())) {
#else
	if (FileOutdated(m_iter->path().filename())) {
#endif
		return true;
	}
	return false;
}

bool FileRemover::FileOutdated(const string& filename)
{
	int year, month;
#if BOOST_FILESYSTEM_VERSION == 3
	if (not FilenameToYearMonth(m_iter->path().filename().string(), year, month)) {
#else
	if (not FilenameToYearMonth(m_iter->path().filename(), year, month)) {
#endif
		return false;
	}
	int distance = (m_current_year * 12) + m_current_month - (year * 12) - month;
	if (distance > m_months) {
		return true;
	} else {
		return false;
	}
}

bool FileRemover::FilenameToYearMonth(const string& filename, int& year, int& month)
{
	if (filename.length() != 10) return false;
	if (filename.substr(6, 4) != ".szc") return false;
	try {
		year = lexical_cast<int>(filename.substr(0, 4));
		month = lexical_cast<int>(filename.substr(4, 2));
		return true;
	} catch (bad_lexical_cast) {
		return false;
	}
}

