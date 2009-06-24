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
 
 * Darek Marcinkiewicz
 *
 * $Id$
 */


#ifndef __KOPERFRAME_H__
#define __KOPERFRAME_H__

// For compilers that support precompilation, includes "wx/wx.h".
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/timer.h>
#include <wx/sizer.h>
#else
#include <wx/wxprec.h>
#endif

#include "koperslot.h"
#include "kopervfetch.h"

class KoperFrame : public wxFrame {
protected:
	int m_rx, m_ry;
	KoperSlot *m_slot;
public:
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseMotion(wxMouseEvent& event);
	void OnMouseEnter(wxMouseEvent& event);
	void OnWindowCreate(wxWindowCreateEvent& event);
	void SetWindowShape();
	KoperFrame(KoperValueFetcher *fetcher, 
		const char* font_texture, 
		const char* background_texture,
		const char* cone_texture,
		const char* font_file,
		const char* prefix);

	DECLARE_EVENT_TABLE()
};
#endif
