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

#include <wx/datetime.h>

#include "disptime.h"
#include "cconv.h"

#define TIMEID 3888

IMPLEMENT_DYNAMIC_CLASS(DisplayTimeWidget, wxStaticText)

BEGIN_EVENT_TABLE(DisplayTimeWidget, wxStaticText)
	EVT_TIMER(TIMEID, DisplayTimeWidget::OnTimer)
END_EVENT_TABLE()


DisplayTimeWidget::DisplayTimeWidget(wxWindow* parent, wxWindowID id)
        : wxStaticText(parent, id, _T("wwwwwwwwwwwwwwwwwww\n"), wxDefaultPosition, wxDefaultSize, 
			wxALIGN_CENTER | wxST_NO_AUTORESIZE)
{
	SetHelpText(_T("draw3-base-win"));

	wxFont f = GetFont();
#ifdef __WXGTK__
	f.SetPointSize(f.GetPointSize() - 1);
#endif

	SetFont(f);
	
	m_timer = new wxTimer(this, TIMEID);
	Update();
}

void DisplayTimeWidget::OnTimer(wxTimerEvent& WXUNUSED(event))
{
	Update();
}

void DisplayTimeWidget::Update()
{
	wxDateTime now = wxDateTime::Now();
#ifndef __WXMSW__
	if (wxLocale::GetSystemLanguage() == wxLANGUAGE_POLISH)
#endif
	{
		wxString label = now.Format(_T("%A, %d "));
		wxString pl_month_names[wxDateTime::Inv_Month] = {
			_("january"),
			_("february"),
			_("march"),
			_("april"),
			_("may"),
			_("june"),
			_("july"),
			_("august"),
			_("septermber"),
			_("october"),
			_("november"),
			_("december"),
		};
		label += pl_month_names[now.GetMonth()];
		label += now.Format(_(" %Y%n hour %H:%M"));
		SetLabel(label);
	}
#ifndef __WXMSW__
	else {
		SetLabel(now.Format(_("%A, %d %B %Y%n time %H:%M")));
	}
#endif
		

	/* This is how it should work, but, sadly, it doesn't work in 
	 * Windows build. I have to use only 1 timer per application. */
	m_timer->Start((60 - now.GetSecond()) * 1000, FALSE);
}

