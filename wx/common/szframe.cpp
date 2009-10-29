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

#include "szframe.h"
#include "../../resources/wx/icons/szarp64.xpm"

wxIcon szFrame::default_icon;

szFrame::szFrame(wxWindow* parent, wxWindowID id, const wxString& title, 
		const wxPoint& pos, const wxSize& size, 
		long style, 
		const wxString& name, 
		const char** icon): wxFrame(parent, id, title, pos, size, style, name)
{
	if (icon != NULL) {
		wxIcon ic = wxIcon(icon);
		if (ic.IsOk()) {
			SetIcon(ic);
		}
	} else if (szFrame::default_icon.IsOk()) {
		SetIcon(default_icon);
	} else {
		SetIcon(wxIcon(szarp64_xpm));
	}
#ifdef MINGW32
	SetBackgroundColour(WIN_BACKGROUND_COLOR);
#else
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BACKGROUND));
#endif
	
}

void szFrame::setDefaultIcon(wxIcon icon) {
	default_icon = icon;	
}

