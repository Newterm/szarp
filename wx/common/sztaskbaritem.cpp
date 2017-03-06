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
 */
#include "sztaskbaritem.h"

#ifdef __WXGTK__
#include <gtk/gtk.h>

static void status_icon_on_left_click(GtkStatusIcon *status_icon, 
		                        gpointer user_data) {
	szTaskBarItem* item = (szTaskBarItem*) user_data;

	wxTaskBarIconEvent e(wxEVT_TASKBAR_LEFT_DOWN, NULL);
	item->GetEventHandler()->ProcessEvent(e);

}

static void status_icon_on_right_click(GtkStatusIcon *status_icon, guint button, 
		                       guint activate_time, gpointer user_data) {
	szTaskBarItem* item = (szTaskBarItem*) user_data;

	wxTaskBarIconEvent e(wxEVT_TASKBAR_RIGHT_DOWN, NULL);
	item->GetEventHandler()->ProcessEvent(e);
}


szTaskBarItem::szTaskBarItem() {
	wxTopLevelWindow::Create(
		NULL, wxID_ANY, _T("systray icon"),
		wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR | wxSIMPLE_BORDER |
		wxFRAME_SHAPED,
		wxEmptyString /*eggtray doesn't like setting wmclass*/);

	m_status_icon = gtk_status_icon_new();

	g_signal_connect(G_OBJECT(m_status_icon), "activate", G_CALLBACK(status_icon_on_left_click), this);
	g_signal_connect(G_OBJECT(m_status_icon), "popup-menu", G_CALLBACK(status_icon_on_right_click), this);

}

void szTaskBarItem::SetIcon(const wxIcon &icon, const wxString &tooltip) {
	wxBitmap bmp;
	bmp.CopyFromIcon(icon);

	gtk_status_icon_set_from_pixbuf((GtkStatusIcon*) m_status_icon, bmp.GetPixbuf());
}

void szTaskBarItem::PopupMenu(wxMenu *menu) {
	wxTopLevelWindow::PopupMenu(menu);
}

szTaskBarItem::~szTaskBarItem() {
}

#endif


