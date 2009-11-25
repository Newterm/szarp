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

#ifndef __SINGLEINSTANCE_H__
#define __SINGLEINSTANCE_H__

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/snglinst.h>
#endif

#ifdef MINGW32

class szSingleInstanceChecker : public wxSingleInstanceChecker {
public:
	szSingleInstanceChecker(const wxString& name, 
			const wxString& path = wxEmptyString,
			const wxString& app = wxEmptyString)
		: wxSingleInstanceChecker(name, path)
	{ }
};

#else

/**
 * Under Windows we use normal wxSingleInstanceChecker, under Linux we add 
 * checking for name of process owning lock file.
 * We can't subclass wxSingleInstanceChecker, because m_impl is private, so
 * we have to rewrite it.
 */
class szSingleInstanceChecker : public wxSingleInstanceChecker {
public:
	/**
	 * @param appname name of executable expected to hold lock. Only
	 * filename part is required.
	 */
	szSingleInstanceChecker(const wxString& name, 
			const wxString& path = wxEmptyString,
			const wxString& appname = wxEmptyString)
		: wxSingleInstanceChecker()
	{
		Create(name, path, appname);
	}
	bool Create(const wxString& name,
                                     const wxString& path,
				     const wxString& appname);
	bool IsAnotherRunning() const;
protected:
	wxString m_appname;	/**< executable name */
	wxString m_fullname;	/**< full path to lock file */
};

#endif /* not MINGW32 */

#endif /* __SINGLEINSTANCE_H__ */
