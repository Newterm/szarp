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
 * draw3
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 * Widget for selecting draws.
 */

#ifndef __SELDRAW_H__
#define __SELDRAW_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/validate.h>


/** Validator class - checks if draw can be disabled, and inform
 * drawswdg about enabling/disabling widget */
class SelectDrawValidator : public wxValidator {
	
	//DECLARE_DYNAMIC_CLASS(SelectDrawValidator)
	public:
		
	/** @param drawswdg widget to iteract with
	 * @param draw draw to disable/enable 
	 * @param index index of draw 
	 */
	SelectDrawValidator(DrawsWidget *drawswdg, int index, wxCheckBox *cb);
	/** copying constructor */
	SelectDrawValidator(const SelectDrawValidator& val);
		
	~SelectDrawValidator();
		
	/** Make a clone of this validator (or return NULL) - currently necessary
	 * if you're passing a reference to a validator. */
	virtual wxObject *Clone() const { return new SelectDrawValidator(*this); }
	
	bool Copy(const SelectDrawValidator& val);
	
	/** Called when the value in the window must be validated. */
	virtual bool Validate(wxWindow *parent);
	
	/** Called to transfer data to the window. */
	virtual bool TransferToWindow();

	/** Event handler */
	void OnCheck(wxCommandEvent& c);
	
	/** Called to transfer data to the window. */
	virtual bool TransferFromWindow();

	/** Pops up a menu with "block/unblock draw" checkbox item*/
	void OnMouseRightDown(wxMouseEvent &event);

	/** Pops up a menu with "block/unblock draw" checkbox item*/
	void OnMouseMiddleDown(wxMouseEvent &event);

	void Set(DrawsWidget *drawswdg, int index, wxCheckBox *cb);

	protected:
	DECLARE_EVENT_TABLE()

	wxMenu *m_menu;		/**menu with item allowing user to block a draw*/

	wxCheckBox *m_cb;	/**poineter do draw's checkbox, for menu popup*/
	
	DrawsWidget *m_draws_wdg;/** pointer to draws widget, we have to communicate with this object */
	int m_index;		/** index of draw to validate */
   
};

/**
 * Widget for selecting draws.
 */
class SelectDrawWidget: public wxWindow, public DrawObserver, public ConfigObserver
{
public:
	SelectDrawWidget() : wxWindow()
	{ }
	/**
	 * @param cfg configuration manager
	 * @param confid identifier (title) of configuration
	 * @param widget for selecting set of draws, needed for getting
	 * currently selected set
	 * @param widget for drawing draws
	 * @param parent parent widget
	 * @param widget id
	 */
	SelectDrawWidget(ConfigManager *cfg, DatabaseManager *dbmgr, DrawsWidget *drawswdg, wxWindow *parent, wxWindowID id = -1);
	/**
	 * Enables/disables draw
	 * @param index draw index 
	 * @param enable if true draw will be enabled, if false disabled*/

	void SetDrawEnable(int index, bool enable);
	
	void SetChecked(int idx, bool checked);

	void SetBlocked(int idx, bool blocked);

	void BlockedChanged(Draw *draw);

	void EnableChanged(Draw *draw);

	void OpenParameterDoc(int i = -1);	

	virtual void DrawInfoChanged(Draw *draw);
protected:
	/** configuration manager */
	ConfigManager *m_cfg;	
	/** database manager */
	DatabaseManager *m_dbmgr;
	/** configuration prefix */
	wxString m_prefix;
	/** widget for drawing draws */
	DrawsWidget *m_draws_wdg;
	/** array of checkboxes */
	wxCheckBox m_cb_l[MAX_DRAWS_COUNT];

	/**return number of selected draw*/
	int GetClicked(wxCommandEvent &event);

	/**Blocks, unblocks a draw*/
	void OnBlockCheck(wxCommandEvent &event);

	void SetChanged(DrawsController *draws_controller);
	
	/** sets parameter */
	void OnPSC(wxCommandEvent &event);

	/**Blocks, unblocks a draw*/
	void OnDocs(wxCommandEvent &event);

	/** edit parametr */
	void OnEditParam(wxCommandEvent &event);

        DECLARE_DYNAMIC_CLASS(SelectDrawWidget)
        DECLARE_EVENT_TABLE()
};


#endif

