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
 * SZARP About dialog.
 *

 * 
 * $Id$
 */

#ifndef __ABOUTDLG_H__
#define __ABOUTDLG_H__

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class szAboutDlg : public wxDialog {
	DECLARE_EVENT_TABLE();
	void OnOKButton(wxCommandEvent& event);
	void OnLink(wxTextUrlEvent& event);
	wxTextCtrl* m_textCtrl;	/**< Main about text control */
public:
	szAboutDlg(wxBitmap* bitmap, wxString programName, wxString version, wxString releasedate, wxArrayString authors, wxWindow *parent = NULL);
};

enum {
	ID_ABOUT_DLG_OK_BUTTON = wxID_HIGHEST,
	ID_ABOUT_TEXT,
	ID_ABOUT_PRATERM,
	ID_ABOUT_SZARP,
};

#endif /* __ABOUTDLG_H__ */

