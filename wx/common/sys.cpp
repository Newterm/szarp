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


#include "sys.h"
#include <sys/resource.h>
#include <cerrno>
#include <array>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

/* set soft core ulimit on hard core ulimit ( max that can be set as non-root user ) */
void SetMaxCoreDumpLimit(){

	struct rlimit core_limit;

	if (getrlimit(RLIMIT_CORE, &core_limit) != 0) {
		std::array<char, 256> buf;
		wxLogError(_T("failed get core limit: %s"), strerror_r( errno, buf.data(), buf.size()));
	} else {
		wxLogInfo(_T("success: get soft core ulimit soft=%lu hard=%lu"), core_limit.rlim_cur, core_limit.rlim_max);
	}

	core_limit.rlim_cur = core_limit.rlim_max;
	if (setrlimit(RLIMIT_CORE, &core_limit) != 0) {
		std::array<char, 256> buf;
		wxLogError(_T("failed set soft core to %lu: %s"), core_limit.rlim_cur, strerror_r( errno, buf.data(), buf.size() ));
	} else {
		wxLogInfo(_T("success: set soft core ulimit to %lu"), core_limit.rlim_cur);
	}
}
