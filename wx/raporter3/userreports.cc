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
/* $Id$
 *
 * SZARP
 * Saving/loading/getting list of user raports
 */

#include "userreports.h"
#include "conversion.h"

wxString UserReports::CreateDir()
{
	if (m_dir.DirExists()) {
		if (!m_dir.IsDirWritable()) {
			return wxString(_("Directory is not writable: ")) +
				m_dir.GetFullPath();
		}
	} else if (!m_dir.Mkdir(0775, wxPATH_MKDIR_FULL)) {
		return wxString(_("Cannot create directory: ")) + m_dir.GetFullPath();
	}
	return wxEmptyString;
}

bool UserReports::CheckFile(wxString filename)
{
	filename = filename.BeforeLast('.');
	szParList parlist;
	if (parlist.LoadFile(GetTemplatePath(filename), false) <= 0) {
		return false;
	}
	if (parlist.GetConfigName().Cmp(m_config) != 0) {
		return false;
	}
	if (parlist.GetExtraRootProp(SZ_REPORTS_NS_URI, _T("title")).Cmp(filename) != 0) {
		return false;
	}
	return true;
}


wxString UserReports::SaveTemplate(wxString name, szParList parlist)
{
	  wxString err;
	  err = CreateDir();
	  if (!err.IsEmpty()) {
		  return err;
	  }
	  parlist.AddNs(_T("rap"), SZ_REPORTS_NS_URI);
	  parlist.SetExtraRootProp(SZ_REPORTS_NS_URI, _T("title"), name);
	  wxFileName path = wxFileName(m_dir.GetFullPath(), name + _T(".xpl"));
	  if (!parlist.SaveFile(path.GetFullPath().c_str())) {
		  return wxString(_("Error saving file: ")) + path.GetFullPath();
	  }
	  if (m_list.Index(name) == wxNOT_FOUND) {
		  m_list.Add(name);
	  }
	  return wxEmptyString;
}

void UserReports::RemoveTemplate(wxString name)
{
	  int i = m_list.Index(name);
	  assert (i != wxNOT_FOUND);
	  m_list.RemoveAt(i);
	  wxFileName path = wxFileName(m_dir.GetFullPath(), name + _T(".xpl"));
	  wxRemoveFile(path.GetFullPath());
}

void UserReports::RefreshList(wxString config)
{
	  m_list.Empty();
	  if (!config.IsEmpty()) {
		  m_config = config;
	  }
	  if (m_config.IsEmpty()) {
		  return;
	  }
	  wxDir d(m_dir.GetFullPath());
	  if (!d.IsOpened()) {
		  return;
	  }
	  wxString filename;
	  bool cont = d.GetFirst(&filename, _T("*.xpl"), wxDIR_FILES);
	  while (cont) {
		  if (CheckFile(filename)) {
			  m_list.Add(filename.BeforeLast('.'));
		  }
		  cont = d.GetNext(&filename);
	  }
}

wxString UserReports::GetTemplatePath(wxString name)
{
	return m_dir.GetFullPath() + wxFileName::GetPathSeparator() + name + _T(".xpl");
}

bool UserReports::FindTemplate(wxString name, wxString& config)
{
	szParList parlist;
	if (parlist.LoadFile(GetTemplatePath(name), false) <= 0)
		return false;
	if (parlist.GetExtraRootProp(SZ_REPORTS_NS_URI, _T("title")).Cmp(name) != 0) {
		return false;
	}

	if (parlist.GetConfigName().Cmp(m_config) == 0) {
		config.Empty();
	} else {
		config = parlist.GetConfigName();
	}
	return true;
}

