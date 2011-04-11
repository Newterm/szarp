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

#ifndef _BALLOONTASKBARITEM_H__
#define _BALLOONTASKBARITEM_H__

#ifdef __WXMSW__

#define _WIN32_IE 0x600
#include <wx/platform.h>
#include <winsock2.h>
#include <sys/types.h>
#include <shlwapi.h>

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/snglinst.h>
#endif

#include <wx/config.h>
#include <wx/taskbar.h>


#define __HAS_BALLOON__ 1

/** A bit of black magic. Declaration of class
 * that is defined by wxWidgets. Idea stolen
 * from wxMusik application. It problably won't 
 * be neccesary in future realases of wxWidgets.*/
class wxTaskBarIconWindow: public wxFrame {
public:
	wxTaskBarIconWindow();

	WXLRESULT MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam);

};

/**Taskbar icon that is able to display balloon tips.
 * Windows only*/
class BalloonTaskBar: public wxTaskBarIcon {
public:
	static const int ICON_ERROR;
	static const int ICON_INFO;
	static const int ICON_NONE;
	static const int ICON_WARNING;

	#if _WIN32_IE>=0x0600
	static const int ICON_NOSOUND; // _WIN32_IE=0x0600
	#endif
public:

	/**Displays balloon 
	 * @param title title of the balloon
	 * @param msg message to be displayed
	 * @param icon icon type
	 * @param timeout amount of time the balloon shall be displayed*/
	void ShowBalloon(wxString title, wxString msg, int iconID = ICON_INFO, unsigned int timeout = 10000);
};

#endif

#endif


