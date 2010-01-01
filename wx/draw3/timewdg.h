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
 */

#ifndef __TIMEWDG_H__
#define __TIMEWDG_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#include <functional>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * Widget for selecting period of time displayed in draws widget.
 */
class TimeWidget: public wxRadioBox, public DrawObserver
{
public:
        /** Constructor.
         * @param parent window
         * @param id window's identifier
         * @draws pointer to corresponding draws widget.
         */
        TimeWidget(wxWindow* parent, DrawsWidget *draws_widget, PeriodType pt);
        /** Event handler, called when radio button is selected.
         * If different item is selected, draws widget is notified and
         * forced to redraw.
         */
        void OnRadioSelected(wxCommandEvent& event);

	/** Select item and (optionnaly) update 
	 * @param item number of item to be selected
	 */ 
        void Select(int item, bool refresh = true);
	
	/** Select previously selected item and refresh DrawsWidget */
        int SelectPrev();

	virtual void PeriodChanged(Draw *draw, PeriodType pt);

	virtual void DrawInfoChanged(Draw *draw);
protected:
	DrawsWidget* m_draws_widget;      /**< Corresponding draws widget. */
        int m_previous;			/**< Previously selected item. */
        int m_selected;                   /**< Currently selected item. */

        DECLARE_EVENT_TABLE()
};


#endif

