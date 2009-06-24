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
 * scc - Szarp Control Center
 * SZARP
 
 *
 * $Id$
 */
#ifndef __SPLASHSCREEN_H__
#define __SPLASHSCREEN_H__

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
#include <wx/statline.h>
#endif

#ifdef new
#undef new 
#endif

#include "drawapp.h"
#include "imagepanel.h"

/**Frame displaying spalsh screen*/
class SplashScreen : public wxDialog
{
public:
	enum {
		ID_SplashScreen = wxID_HIGHEST + 200,
	};

	SplashScreen(wxBitmap *bitmap = NULL);
	virtual ~SplashScreen() {};

	/**Panel displaying bimap*/
	ImagePanel *img;

	void PushStatusText(wxString status);
	void PopStatusText();

private:
	wxStatusBar *status_bar;	/*<frame's status bar widget*/
};


#endif
