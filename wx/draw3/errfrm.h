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
 * $Id$
 */

#ifndef __ERRFRM_H__
#define __ERRFRM_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


class ErrorFrame : public wxFrame {
	void OnHide(wxCommandEvent &event);
	void OnClear(wxCommandEvent &event);
	void OnClose(wxCloseEvent &event);

	wxTextCtrl *m_text;
	wxCheckBox *m_rise_check;

	void AddErrorText(const wxString &s);

	static void Create();
	static ErrorFrame *_error_frame;

	void OnHelpButton(wxCommandEvent &event);
	DECLARE_EVENT_TABLE()
public:
	ErrorFrame();
	virtual ~ErrorFrame();
	static void NotifyError(const wxString &s);
	static void DestroyWindow();
	static void ShowFrame();
};

#endif
