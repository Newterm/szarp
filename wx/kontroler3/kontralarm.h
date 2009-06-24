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
 * kontroler3 program
 * SZARP

 * ecto@praterm.com.pl
 */

#ifndef _KONTRALARM_H
#define _KONTRALARM_H

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "parlist.h"

class szKontroler;

/** Kontroler: add par */
class szAlarmWindow : public wxFrame {
	szKontroler *m_kontroler_frame;
	wxString m_param_name;
public:
	/**
	* @param _ipk szarp config
	*/
	szAlarmWindow(szKontroler *kontroler_frame, const double& val, szProbe* probe);
	virtual ~szAlarmWindow() {};
private:
	void OnOK(wxCommandEvent &event);
	DECLARE_EVENT_TABLE()
};

#endif //_KONTRADD_H
