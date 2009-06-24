/* $Id$
# SZARP: SCADA software 
# 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * raporter3 program
 * SZARP

 * ecto@praterm.com.pl
 */

#ifndef _PARCALC_H
#define _PARCALC_H

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/treectrl.h>

#include <szarp_config.h>

#include "parselect.h"

/** */
class szParCalc : public wxDialog {
  DECLARE_DYNAMIC_CLASS(szParCalc)
  DECLARE_EVENT_TABLE()
  /** */
  TSzarpConfig *ipk;
  /** */
  szParSelect *ps;
  /** */
  wxTextCtrl *display_txtc;
public:
  struct {
    wxString m_form;
  } g_data;
  /** */
  szParCalc() {}
  /** */
  szParCalc(TSzarpConfig *ipk,
      wxWindow* parent, wxWindowID id, const wxString& title);
  virtual ~szParCalc();
  /** */
protected:
  /** */
  void OnButtonEntry(wxCommandEvent &ev);
};

/*
 * IDs
 */
enum {
  ID_TC_DISP = wxID_HIGHEST + 1,
  ID_B_N0,
  ID_B_N1,
  ID_B_N2,
  ID_B_N3,
  ID_B_N4,
  ID_B_N5,
  ID_B_N6,
  ID_B_N7,
  ID_B_N8,
  ID_B_N9,
  ID_B_OPLUS,
  ID_B_OMINUS,
  ID_B_OMUL,
  ID_B_ODIV,
  ID_B_CAC,
  ID_B_SPARAM
};


#endif //_PARCALC_H
