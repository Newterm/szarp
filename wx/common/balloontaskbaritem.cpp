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

 * schylek@praterm.com.pl
 */

#include "balloontaskbaritem.h"

#ifdef __WXMSW__

const int BalloonTaskBar::ICON_ERROR = NIIF_ERROR;
const int BalloonTaskBar::ICON_INFO = NIIF_INFO;
const int BalloonTaskBar::ICON_NONE = NIIF_NONE;
const int BalloonTaskBar::ICON_WARNING = NIIF_WARNING;

#if _WIN32_IE>=0x0600
const int BalloonTaskBar::ICON_NOSOUND = NIIF_NOSOUND; // _WIN32_IE=0x0600
#endif

void BalloonTaskBar::ShowBalloon(wxString title, wxString msg, int
		    iconID, unsigned int timeout) {

	wxConfigBase* config = wxConfigBase::Get(true);

	long show_notification = config->Read(_T("SHOW_NOTIFICATION"), 1L);

	if (show_notification == 0)
		return;

        NOTIFYICONDATA nid;
        memset(&nid,0,sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = (HWND)m_win->GetHWND();
        nid.uID = 99;
        nid.uFlags = NIF_INFO;
        wxStrncpy(nid.szInfo, msg.c_str(), WXSIZEOF(nid.szInfo));
        wxStrncpy(nid.szInfoTitle, title.c_str(), WXSIZEOF(nid.szInfoTitle));
        nid.dwInfoFlags = iconID | NIIF_NOSOUND;
        nid.uTimeout = timeout;

        Shell_NotifyIcon(NIM_MODIFY, &nid);

}

#endif

