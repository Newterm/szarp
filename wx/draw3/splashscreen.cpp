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


#include "szframe.h"

#include "ids.h"
#include "classes.h"
#include "splashscreen.h"
#include "drawapp.h"

SplashScreen::SplashScreen(wxBitmap *bitmap) : wxDialog(NULL, ID_SplashScreen, wxString(_("Starting draw3...")), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	SetIcon(szFrame::default_icon);

	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

	if(bitmap) {
		img = new ImagePanel(this, *bitmap);
		sizer->Add(img, 0, wxALIGN_CENTER);
	}

	status_bar = new wxStatusBar(this);
	sizer->Add(status_bar, 0, wxALIGN_CENTER);

	int w, h;
	status_bar->GetClientSize(&w, &h);

	if(bitmap) {
		SetClientSize(bitmap->GetWidth(), bitmap->GetHeight());
		status_bar->SetSize(bitmap->GetWidth(), h);
	} else
		SetClientSize(w, h);



	Centre();

	wxGetApp().Yield();
}

void
SplashScreen::PopStatusText()
{
	this->status_bar->PopStatusText();
	wxGetApp().Yield();
	this->Refresh();
	this->Update();
}

void
SplashScreen::PushStatusText(wxString status)
{
	this->status_bar->PushStatusText(status);
	wxGetApp().Yield();
	this->Refresh();
	this->Update();
}

