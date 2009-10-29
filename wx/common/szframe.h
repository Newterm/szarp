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

#ifndef __SZFRAME_H
#define __SZFRAME_H

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef MINGW32
// Windows port sets strange default background for frames; there seems to be no
// way to get some reasonable value from system setting or any other source;
// these values are probed from probed from some Windows XP application
#define WIN_BACKGROUND_COLOR	wxColour(236, 233, 216, 255)
#endif

/**
 * Parent class for all frames - sets application icon.
 * First, set default application icon:
 * 	szFrame::setDefaultIcon(icon);
 * Then use szFrame as parent class for your frames:
 *	class MyFrame : szFrame { };
 * or set icon manually:
 * 	dialog->SetIcon(szFrame::default_icon);
 */
class szFrame : public wxFrame {
public:
	static wxIcon default_icon;	/**< default application icon */
	/**
	 * Calls wxFrame constructor and sets default_icon as application icon.
	 */
	szFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE, const wxString& name = _T("frame"), const char** icon = NULL);
	/** Set default application icon.
	 * @param icon default application icon
	 */
	static void setDefaultIcon(wxIcon icon);
};

#endif /* __SZFRAME_H */

