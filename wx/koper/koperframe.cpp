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
 * koper - Kalkulator Premii
 * SZARP
 
 * Dariusz Marcinkiewicz
 *
 * $Id$
 */

#include "koperframe.h"

BEGIN_EVENT_TABLE(KoperFrame, wxFrame)
	EVT_ENTER_WINDOW(KoperFrame::OnMouseEnter)
	EVT_WINDOW_CREATE(KoperFrame::OnWindowCreate)
END_EVENT_TABLE()

KoperFrame::KoperFrame(KoperValueFetcher* fetcher, 
		const char* text_texture, 
		const char* background_texture,
		const char* cone_texture,
		const char* font_file,
		const char* prefix) :
       	wxFrame(NULL, wxID_ANY, _T("KoPer"), wxPoint(100,100), 
#if 1
			wxSize(180,70),
#else
			wxSize(200,200), 
#endif
			wxNO_BORDER | wxSTAY_ON_TOP | wxFRAME_SHAPED) {
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);	
	m_slot = new KoperSlot(this, text_texture, background_texture, cone_texture, font_file, prefix, fetcher);
	sizer->Add(m_slot, 0, 0, 0, 0);
	SetSizer(sizer);
}

void KoperFrame::OnMouseRightDown(wxMouseEvent& event) {
	m_rx = event.m_x;
	m_ry = event.m_y;
}

void KoperFrame::OnMouseEnter(wxMouseEvent& event) {
	SetFocus();
}

void KoperFrame::OnMouseMotion(wxMouseEvent& event) {
	wxPoint p = GetPosition();
	Move(p.x + event.GetX() - m_rx, p.y + event.GetY() - m_ry);
}


void KoperFrame::OnWindowCreate(wxWindowCreateEvent& WXUNUSED(event)) {
	SetWindowShape();
}

void KoperFrame::SetWindowShape() {

//bleee, setting shape does not work, example applciation from wx2.6 that
//creates non-rectangular window does not work either...
#if 0
	const unsigned np = 100;

	wxPoint points[np + 1];

	const wxSize& size = GetSize();
	int w = size.GetWidth();
	int h = size.GetHeight();
	int r = (int) sqrtf(w * w + h * h);

	for (unsigned i = 0; i <= np; i++) {
		int x = (int) (r * cos(2.f * M_PI * i / np));
		int y = (int) (r * sin(2.f * M_PI * i / np));
		points[i] = wxPoint(x,y);
	}

	SetShape(wxRegion(np + 1, points));
#endif

}
