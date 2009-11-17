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
 * raporter3 program
 * SZARP

 * ecto@praterm.com.pl
 */

#ifndef _PARSELECT_H
#define _PARSELECT_H

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#ifndef __SZHLPCTRL_H__
#include "szhlpctrl.h"
#endif
#include <wx/cshelp.h>
#include <wx/treectrl.h>
#include "szarp_config.h"

/** szParSelectEvent events are generated when user adds parameter in 
 * 'add-style' szParSelect widget. Use someting like this in your code:
 * BEGIN_EVENT_TABLE(MyFrame, wxFrame)
 * 	EVT_SZ_PARADD(window_id, MyFrame::method)
 * END_EVENT_TABLE
 */

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_LOCAL_EVENT_TYPE(wxEVT_SZ_PARADD, wxID_HIGHEST + 2001)
END_DECLARE_EVENT_TYPES()

/** Class ArrayOfTParam is array of TParam pointers */
WX_DEFINE_ARRAY(TParam *, ArrayOfTParam);

class szParSelectEvent:public wxCommandEvent {
 public:
	/** @param params array of selected params */
	szParSelectEvent();
	szParSelectEvent(ArrayOfTParam params,
			 int id = 0, 
			 wxEventType 
			 eventType = wxEVT_NULL);
	/** Copy constructor */
	 szParSelectEvent(const szParSelectEvent & event);
	/** Returns pointer to copy of event. Every event class has to
	 * implement Clone() method.
	 */
	virtual wxEvent *Clone() const;
	/** @return pointer to array of selected parameters */
	const ArrayOfTParam *GetParams();
	/** Sets array of parameters. Used on event creating. */
	void SetParams(ArrayOfTParam & params);
 protected:
	 ArrayOfTParam params;	/**< Array of pointers to selected parameters. */

};

#if wxCHECK_VERSION(2,5,0)

typedef void (wxEvtHandler::*szParSelectEventFunction) (szParSelectEvent &);
#define EVT_SZ_PARADD(id, fn) \
  DECLARE_EVENT_TABLE_ENTRY( \
          wxEVT_SZ_PARADD, \
          id, wxID_ANY, \
          (wxObjectEventFunction)(wxEventFunction) (wxCommandEventFunction) \
            wxStaticCastEvent( szParSelectEventFunction, &fn ), \
          (wxObject *) NULL \
  ),
#else				//wxCHECK_VERSION
#define EVT_SZ_PARADD(id, fn) \
  DECLARE_EVENT_TABLE_ENTRY( \
          wxEVT_SZ_PARADD, \
          id, wxID_ANY, \
          (wxObjectEventFunction)(wxEventFunction) \
            (wxCommandEventFunction) &fn, \
          (wxObject *) NULL \
  ),
#endif				//wxCHECK_VERSION

/**
 * Widget for selecting parameters from tree. ShowModal() method should be 
 * used for displaying widget. There are two styles of widgets. With first 
 * style user has to select parameter and then click 'Ok' button. ShowModal()
 * returns then with wxID_OK and selected parameter is in g_data member.
 * With 'add' style you can use Show() or ShowModal(). There are buttons 
 * 'Add' and 'Close'. When user clicks 'Add' szParSelectEvent is generated.
 */
class szParSelect:public wxDialog {
	DECLARE_DYNAMIC_CLASS(szParSelect)
	DECLARE_EVENT_TABLE()
 public:
	szParSelect():wxDialog() { };
      	/** Type of functions for filtering params. */
	typedef int (szParFilter) (TParam *);
      	/**
	* @param ipk pointer to IPK object
	* @param parent parent widget
	* @param id window ID
	* @param title window caption text
	* @param addStyle TRUE if 'add' style should be used
	* @param multiple TRUE if multiple choice is allowed, ignored if addStyle is
	* FALSE because normal style allows to select only one parameter
	* @param showShort TRUE if we should display short name of selected parameter,
	* ignored if multiple is TRUE
	* @param showDescr TRUE if we should display description (draw name) of selected 
	* @param _single
	* @param filter parameter filter - only parameters for which function
	* @param description_is_param_name - if set to true as descrption is treated
	* last parm of long param name, otherwise it is draw name
	* returns 0 are loaded to tree, default is NULL - all parameters are
	* accepted
	* parameter ignored if multiple is TRUE
	*/
	szParSelect(TSzarpConfig * ipk,
	    		wxWindow * parent, wxWindowID id, const wxString & title,
	    		bool addStyle = FALSE,
	    		bool multiple = TRUE,
	    		bool showShort = TRUE,
	    		bool showDescr = TRUE,
	    		bool _single = FALSE,
	    		szParFilter * filter = NULL, bool descriptionIsParmName = false);
      	/** return data - set after return from Show() or ShowModal() if they
	 * return wxID_OK */
	struct {
		TParam *m_param;
		wxString m_scut;
		wxString m_desc;
	} g_data;

	void OnParSelect(wxTreeEvent & ev);
	void OnParChanging(wxTreeEvent & ev);
	void OnAddClicked(wxCommandEvent & ev);
	void OnAddTreeClicked(wxTreeEvent & ev) 
	{
		OnAddClicked(ev);
	}
	void OnCloseClicked(wxCommandEvent & ev);
	void OnHelpClicked(wxCommandEvent & ev);
	void OnCheckClicked(wxCommandEvent & ev);
	/**
	 *  Function set the context text which is the name of the section in book,
	 *  and set the HelpController from main program.
	 */
	void SetHelp(const wxString & page);
	
 private:
	bool description_is_param_name;
	wxButton *help_button;	 	/**< Help button. */
	szHelpController *main_help;	/**< Pointer on help from main program. */
	wxBoxSizer *button_sizer;   	/**< Pointer on all buttons, later added help_button.*/
	wxCheckBox *check_box;		/**< Checkbox changing tree display method. */
	wxStaticBoxSizer *par_sizer;
	wxString last_param;
	bool single;
 protected:

      	/** load IPK to tree */
	void LoadParams();
      	/** Function load parameters like in draw program */
	void LoadParamsLikeInDraw();
      	/** helper method for LoadParams() - recursively sorts tree
	 * @param id id of subtree to sort */
	void SortTree(wxTreeItemId id);
      	/** Expand tree to previous selected item */
	void ExpandToLastParam(wxTreeItemId looked);
      	/** tree widget  */
	wxTreeCtrl *par_trct;

	wxTextCtrl *scut_txtc;
	wxTextCtrl *desc_txtc;
      	/** pointer to IPK object */
	TSzarpConfig *ipk;
	bool m_is_multiple;
      	/** Parameters filter */
	szParFilter *m_filter;
};

/*
 * IDs
 */
enum {
	ID_TRC_PARS,
	ID_TC_PAR_SCUT,
	ID_TC_PAR_DESC,
	ID_B_PAR_ADD,
	ID_B_PAR_END,
};

#endif	//_PARSELECT_H

