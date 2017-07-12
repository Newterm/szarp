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

#ifndef _REPORTEREDIT_H
#define _REPORTEREDIT_H

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/listctrl.h>

#include <szarp_config.h>

#include "parselect.h"

#include "report.h"

/** Report editor dialog */
class szReporterEdit:public wxDialog {
	DECLARE_CLASS(szReporterEdit)
	DECLARE_EVENT_TABLE()
      	/** szarp config */
	TSzarpConfig *ipk;
      	/** */
	wxListCtrl *rcont_listc;
      	/** */
	szParSelect *ps;

 public:
	 szReporterEdit(TSzarpConfig * _ipk, ReportInfo& report, wxWindow * parent, wxWindowID id);

	ReportInfo& report;

 protected:
        /** event: button: add */
	void OnParAdd(wxCommandEvent & ev);
      	/** event: button: add */
	void OnParDel(wxCommandEvent & ev);
      	/** event: rcont_listc: column drag */
	void OnListColDrag(wxListEvent & ev);
      	/** event: report title modified */
	void OnTitleChanged(wxCommandEvent &ev);
      	/** refresh list view */
	void RefreshList();
	/** enable/disable Ok button based on others controls state */
	void EnableOkButton();
	virtual bool TransferDataToWindow();

	wxWindow *m_okButton;
	wxTextCtrl *m_title;
	wxString report_name;
};

/*
 * IDs
 */
enum {
	ID_B_ADD = wxID_HIGHEST + 1,
	ID_B_DEL,
	ID_LC_RCONT,
	ID_T_TITLE
};

#endif				//_REPORTEREDIT_H
