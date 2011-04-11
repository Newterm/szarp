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
 * confedit - SZARP configuration editor
 * pawel@praterm.com.pl
 *
 * $Id$
 */

#ifndef __DLISTCTRL_H__
#define __DLISTCTRL_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <libxml/tree.h>
#include "deditdlg.h"

class DrawsListCtrl : public wxListCtrl {
  public:
	DrawsListCtrl(wxWindow* parent, wxWindowID id, 
			const wxPoint& pos = wxDefaultPosition, 
			const wxSize& size = wxDefaultSize, 
			long style = wxLC_SORT_ASCENDING | wxBORDER_SUNKEN);

	/** Add draw to list.
	 * @param node pointer to draw XML node
	 */
	long InsertItem(xmlNodePtr node);
	/** Runs editor for draw properties. */
	void OnDrawEdit(wxListEvent &ev);
	/** Move selected item one position up. */
	void MoveItemUp();
	/** Move selected item one position up. */
	void MoveItemDown();
	/** Set colors for draws in list.
	 * Must be called after adding draws or changing order or color property.
	 */
	void AssignColors();
	/** Set items order according to 'order' attributes. */
	void SortItems();
	/** Return position of currently selected item.
	 * @return 0-based position of selected item, -1 if no item is selected
	 */
	long GetSelection();

	bool Modified() { return modified; }
  protected:
	/** Change position of item.
	 * @param from index of item to move
	 * @param to target position of item
	 */
	void MoveItem(long from, long to);
	/** Callback for sorting items.
	 * item1 and item2 arguments are really xmlNode pointers
	 */
	int static wxCALLBACK sort_callback(long item1, long item2, long ignored);

	bool modified;
};



#endif
