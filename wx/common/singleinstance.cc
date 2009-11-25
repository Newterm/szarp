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
 * Modified implementation of wxSingleInstanceChecker - checks also the executable
 * name of process holding lock.
 * 2009 pawel@praterm.com.pl
 * $Id$
 */

#ifndef MINGW32

#include "singleinstance.h"
#include "cconv.h"
#include <wx/file.h>

bool szSingleInstanceChecker::Create(const wxString& name,
                                     const wxString& path,
				     const wxString& appname)
{
    m_appname = appname;

    m_fullname = path;
    if (m_fullname.empty())     {
        m_fullname = wxGetHomeDir();
    }
    if (m_fullname.Last() != _T('/')) {
        m_fullname += _T('/');
    }
    m_fullname << name;
    return wxSingleInstanceChecker::Create(name, path);
}

	
bool szSingleInstanceChecker::IsAnotherRunning() const
{
	bool ret = wxSingleInstanceChecker::IsAnotherRunning();
	if (!ret) {
		return false;
	}
	if (m_appname.IsEmpty()) {
		return true;
	}

	/* if m_impl wasn't private, we could just check for m_impl->GetLockerPID()... */
	wxFile file(m_fullname, wxFile::read);
	if (!file.IsOpened()) {
		wxLogError(_T("szSingleInstanceRunning: opening lock file '%s' failed"), 
				m_fullname.c_str());
		return false;
	}
	char buf[256];
	pid_t pid;
	ssize_t count = file.Read(buf, WXSIZEOF(buf));
	if (count == wxInvalidOffset) {
		wxLogError(_("Failed to read PID from lock file."));
		return false;
	}
	if (sscanf(buf, "%d", (pid_t *)&pid) != 1) {
		wxLogWarning(_("szSingleInstanceRunning: invalid lock file '%s'."), 
				m_fullname.c_str());
		return false;
	}

	if (pid == 0) {
		return false;
	}

	char *path;
	asprintf(&path, "/proc/%d/cmdline", pid);
	assert (path != NULL);
	int fd = open(path, O_RDONLY);
	free(path);
	if (fd == -1) {
		return false;
	}
	buf[sizeof(buf) -1] = 0;
	read(fd, buf, sizeof(buf) - 1);
	close(fd);
	wxString cmdline(SC::A2S(buf));
	if (cmdline.EndsWith(m_appname)) {
		return true;
	} else {
		return false;
	}
}

#endif /* not MINGW32 */

