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
 
  Pawe³ Pa³ucha pawel@praterm.com.pl
  $Id$

  ISL Editor - Inkscape plugin for editing isl namespace properties in ISL schemas.

*/

#include <wx/wx.h>
#include <wx/image.h>
#include "parselect.h"

#ifndef ISLEDIT_H
#define ISLEDIT_H

/**
  Main program frame.
  */
class ISLFrame: public wxFrame {
public:
	/** Constructor.
	  @param ipk pointer to source IPK file
	 */
	ISLFrame(wxWindow* parent, int id, const wxString& title, 
		    TSzarpConfig* ipk,
		    xmlNodePtr element,
		    const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, 
		    long style=wxDEFAULT_FRAME_STYLE);

private:
	/** Set main frame properties */
	void set_properties();
	/** Layout widgets. */
	void do_layout();
	/** Enable/disable element basing on checkbox/radiobutton states. */
	void do_disable();

protected:
	wxTextCtrl* parameter_txtctrl;	/**< textbox with selected parameter name */
	wxButton* parameter_bt;		/**< button - opens dialog for selecting parameter */
	wxRadioBox* content_rb;		/**< radiobutton - select editing of attribute or text
					  content */
	wxCheckBox* v_u_cb;		/**< checkbox - selects @value or @v_u */
	wxStaticText* attribute_lb;	/**< static label */
	wxComboBox* combo_box_1;	/**< combo box for selecting attribute to replace */
	wxStaticText* scale_lb;		/**< static label */
	wxTextCtrl* scale_ctrl;		/**< textbox for scale attribute */
	wxStaticText* shift_lb;		/**< static label */
	wxTextCtrl* shift_ctrl;		/**< textbox for shift attribute */
	wxButton* ok_bt;		/**< ok button */
	wxButton* cancel_bt;		/**< cancel button */
	wxButton* help_bt;		/**< help button */
	szParSelect* parsel_wdg;	/**< widget for selecting parameter */
	
	xmlNodePtr element;		/**< pointer to edited node */
	
	int content_rb_selection;	/**< Selection set by user in content_rb radiobox, 
					  may be changed when disabling widget. */
	DECLARE_EVENT_TABLE();

public:
	void doChoose(wxCommandEvent &event);	/**< Event handler */ 
	void doRadio(wxCommandEvent &event);	/**< Event handler */
	void doCheck(wxCommandEvent &event);	/**< Event handler */ 
	void doOk(wxCommandEvent &event);	/**< Event handler */ 
	void doCancel(wxCommandEvent &event);	/**< Event handler */ 
	void doExit(wxCloseEvent &event);	/**< Event handler */
	void doHelp(wxCommandEvent &event);	/**< Event handler */ 
}; 

#endif // ISLEDIT_H
