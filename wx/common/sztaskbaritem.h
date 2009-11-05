/* SZARP: SCADA software

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
 * SZARP
 * Darek Marcinkiewicz reksio@praterm.com.pl
 *
 * $Id$
 * Incomplete, yet sufficient for our purposes, implementation of a tray icon.
 * The implementation that is currently present in wxGTK uses different api
 * and suffers from known bug - sometimes when program is started by window 
 * managers' autostart scripts the icon does not appear on wm's panel.
 * Note: undes MSW we stick to wxWidgets's standard implementation .
 */
#ifndef __SZTASKBARICON_H__
#define __SZTASKBARICON_H__

#include <wx/wx.h>
#include <wx/taskbar.h>
#include "balloontaskbaritem.h"

#ifdef __WXGTK__


class szTaskBarItem : public wxTopLevelWindow {
	wxTopLevelWindow *m_toplevel;

	void *m_status_icon;
public:
	szTaskBarItem();
	void SetIcon(const wxIcon &icon, const wxString &tooltip = _T(""));
	virtual void PopupMenu(wxMenu *menu);
	virtual ~szTaskBarItem();
};

#else

typedef BalloonTaskBar szTaskBarItem;

#endif

#endif
