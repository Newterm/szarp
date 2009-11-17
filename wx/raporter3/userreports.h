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

#ifndef __USERREPORTS_H__
#define __USERREPORTS_H__

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/filename.h>
#include <wx/dir.h>
#include "parlist.h"
#include "ns.h"

/**
 * Loading and editing list of user reports templates. 
 *
 * Each template is saved to file in ~/.szarp/reports directory. Name of file
 * is identical to the tile of report. File format is *.xpl (szarp XML 
 * parameters list), file also contains report title - if it does not match file 
 * name, file is considered incorrect and is skipped.
 *
 * File also contains information about target SZARP configuration name, only 
 * reports for current configurations are loaded and displayed, but template
 * names must be uniq among all configurations.
 */
class UserReports {
  public:
	  UserReports() 
	  {
		  m_dir.AssignHomeDir();
		  m_dir.AppendDir(_T(".szarp"));
		  m_dir.AppendDir(_T("reports"));
	  }
	  wxArrayString m_list;	/**< list of available reports */
  protected:
	  wxString m_config;	/**< name of current config */ 
	  wxFileName m_dir;	/**< path to directory with reports */

	  /** Makes sure that directory for saving reports exists and
	   * is writeable.
	   * @return error string, empty string on success
	   */
	  wxString CreateDir();
	  /** Check if file contains proper report template for current config. */
	  bool CheckFile(wxString filename);
  public:
	  /** Saves given report template.
	   * @param name of template to save
	   * @parlist list of parameters in report
	   * @return error string, empty string on success
	   */
	  wxString SaveTemplate(wxString name, szParList parlist);
	  
	  /** Remove given template */
	  void RemoveTemplate(wxString name);
	  /** Reloads list of available reports.
	   * @param config name of current config, if empty last config is used
	   */
  	  void RefreshList(wxString config = wxEmptyString);
	  /** Return path for given template.
	   * @param name name of user template
	   * @return full path to file containing template, wxEmptyString on error
	   */
	  wxString GetTemplatePath(wxString name);
	  /** Check if given template exists.
	   * @param name of template to search for
	   * @param returns title of configuration for template found, not modified
	   * if not found, set to wxEmptyString if parameter is found and config
	   * is equal to last config used in RefreshList()
	   * @return true if template was found, false otherwise
	   */
	  bool FindTemplate(wxString name, wxString& config);
};

#endif	// __USERREPORTS_H__
