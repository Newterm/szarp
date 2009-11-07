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

#ifndef _RAPORTEREDIT_H
#define _RAPORTEREDIT_H

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

#include "raporter.h"

/** Raport editor dialog */
class szRaporterEdit:public wxDialog {
	DECLARE_CLASS(szRaporterEdit)
	DECLARE_EVENT_TABLE()
      	/** szarp config */
	TSzarpConfig *ipk;
      	/** */
	wxListCtrl *rcont_listc;
      	/** */
	szParSelect *ps;
 public:
      	/** returned data */
	struct {
	    	/** report title */
		wxString m_report_name;
	    	/** list of reports parameters */
		szParList m_raplist;
	} g_data;
      	/**
	 *  param _ipk szarp config
	 */
	 szRaporterEdit(TSzarpConfig * _ipk,
			wxWindow * parent, wxWindowID id,
			const wxString & title);
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

#endif				//_RAPORTEREDIT_H
