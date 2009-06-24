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
 * draw3
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 * Widget for displaying current time and date.
 */

#ifndef __DISPTIME_H__
#define __DISPTIME_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/stattext.h>
#include <wx/timer.h>

/**
 * Widget for displaying current time and date.
 */
class DisplayTimeWidget: public wxStaticText
{
public:
	DisplayTimeWidget() : wxStaticText()
	{ }
	~DisplayTimeWidget()
	{
		delete m_timer;
	}
	DisplayTimeWidget(wxWindow *parent, wxWindowID id = -1);
	/** Update time info. */
	void Update();
protected:
	/** Event handler, called by timer, calls update. */
	void OnTimer(wxTimerEvent& event);
	/** Timer for updating time */
	wxTimer* m_timer;
        DECLARE_DYNAMIC_CLASS(DisplayTimeWidget)
        DECLARE_EVENT_TABLE()
};


#endif

