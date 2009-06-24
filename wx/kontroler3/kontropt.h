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
 * SZARP

 * ecto@praterm.com.pl
 */

#ifndef _KONTROPT_H
#define _KONTROPT_H

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

/** Kontroler options dialog */
class szKontrolerOpt : public wxDialog {
  DECLARE_CLASS(szKontrolerOpt)
  DECLARE_EVENT_TABLE()
public:
  /** returned data */
  struct {
    int m_per_sec;
    int m_per_min;
    int m_log_len;
  }g_data;
  /** */
  szKontrolerOpt(wxWindow* parent, wxWindowID id, const wxString& title);
  /** event: button: help */
  void OnHelp(wxCommandEvent &ev);
};

#endif //_KONTROPT_H
