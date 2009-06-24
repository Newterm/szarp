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

 * ecto@praterm.com.pl
 */

#ifndef _RAPORTERPERIOD_H
#define _RAPORTERPERIOD_H

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

/** data type/refresh rate dialog */
class szRaporterPeriod : public wxDialog {
  DECLARE_CLASS(szRaporterPeriod)
  DECLARE_EVENT_TABLE()
  void SetPer(int hour, int min, int sec);
public:
  /** returned data */
  struct {
    int m_type;
    int m_per_h, m_per_m, m_per_s;    
  }g_data;
  /** */
  szRaporterPeriod(wxWindow* parent, wxWindowID id, const wxString& title);
protected:
  /** event: button: help */
  void OnHelp(wxCommandEvent &ev);
  /** event: radiobox: change selection */
  void OnTypeChange(wxCommandEvent &ev);
};

/** IDs */
enum {
  ID_RB_PERTYPE = wxID_HIGHEST + 1,
  ID_SL_H,
  ID_SL_M,
  ID_SL_S
};

#endif //_RAPORTERPERIOD_H
