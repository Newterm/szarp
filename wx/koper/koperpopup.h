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

#ifndef __KOPERPOPUP_H__
#define __KOPERPOPUP_H__

// For compilers that support precompilation, includes "wx/wx.h".
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/timer.h>
#include <wx/sizer.h>
#include <wx/datetime.h>
#include <wx/statline.h>
#else
#include <wx/wxprec.h>
#endif

class ClickStaticText;

class KoperPopup : public wxDialog {
	void OnMouseClick(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);
	ClickStaticText* m_valstr1;
	ClickStaticText* m_valstr2;
	ClickStaticText* m_datestr1;
	ClickStaticText* m_datestr2;
	int m_value_prec;
#if 0
	ClickStaticText* m_val1;
	ClickStaticText* m_val2;
	ClickStaticText* m_date1;
	ClickStaticText* m_date2;
#endif
public:
	KoperPopup(wxWindow *parent, const wxString& prefix, int value_prec);
	void SetValues(double v1, const struct tm& t1, double v2, const struct tm& t2);
	DECLARE_EVENT_TABLE()
};

#endif
