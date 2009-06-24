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
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "drawswdg.h"

#define TIME_WIDGET_ID  wxID_HIGHEST

/**
 * Widget for selecting period of time displayed in draws widget.
 */
class TimeWidget: public wxRadioBox
{
public:
        /** Constructor.
         * @param parent window
         * @param id window's identifier
         * @draws pointer to corresponding draws widget.
         */
        TimeWidget(wxWindow* parent, DrawsWidget *draws, PeriodType pt);
        /** Event handler, called when radio button is selected.
         * If different item is selected, draws widget is notified and
         * forced to redraw.
         */
        void OnRadioSelected(wxCommandEvent& event);

	/**transfers focus to @see DrawsWidget*/
	void OnFocus(wxFocusEvent &event);

	/** Select item and (optionnaly) refresh DrawsWidget
	 * @param item number of item to be selected
	 */ 
        void Select(int item, bool refresh = true);
	
	/** Select previously selected item and refresh DrawsWidget */
        int SelectPrev();
protected:
        DrawsWidget* draws_widget;      /**< Corresponding draws widget. */
        int prev;			/**< Previously selected item. */
        int selected;                   /**< Currently selected item. */

        DECLARE_EVENT_TABLE()
};


#endif

