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
 * raporter3 program
 * SZARP

 *
 * $Id$
 */
#ifndef __FILE_DUMP_H__
#define __FILE_DUMP_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef new
#undef new
#endif

#include <wx/filepicker.h>
#include <wx/spinctrl.h>
#include <wx/dir.h>
#include <wx/config.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include "parlist.h"
#include "raporter.h"

#define FILE_DUMP_DIR  _T("FileDump/Dir")
#define FILE_DUMP_ENABLED _T("FileDump/Enabled")
#define FILE_DUMP_MAXSIZE _T("FileDump/MaxSize")

class FileDumpDialog : public wxDialog
{
public:
	enum {
		ID_FileDumpDialog = wxID_HIGHEST + 200,
		ID_DirChanged,
		ID_OkBtn,
		ID_CancelBtn,
	};

	FileDumpDialog(wxFrame *parent, wxString *report_name);
	virtual ~FileDumpDialog()
	{};
	DECLARE_EVENT_TABLE();
private:
	void OnOkBtn(wxCommandEvent& WXUNUSED(event));
	void OnCancelBtn(wxCommandEvent& WXUNUSED(event));
	void OnClose(wxCloseEvent& event);
	void LoadConfiguration();
	void SaveConfiguration();
	void OnUpdateDestination(wxFileDirPickerEvent& event);
	void UpdateDestination();

	wxDirPickerCtrl *dir_picker;
	wxSpinCtrl *spinMB;
	wxCheckBox *enabled_cb;
	wxTextCtrl *destinantion_tb;
	wxButton *ok_btn;
	wxButton *cancel_btn;
	wxString *reportname;
};

class FileDump
{
public:
	static
	void DumpValues(szParList *params, wxString report_name);
	static
	bool DumpEnabled();
	static
	wxFileName GetFileName(wxString report_name);
	static
	wxFileName GetFileName(wxString dir, wxString report_name);
	static
	wxString GetDir();
	static
	long GetMaxSize();
private:
};

#endif
