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

#ifndef __DEDITDLG_H__
#define __DEDITDLG_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "dcolors.h"
#include <wx/tglbtn.h>

/**
 * Editing draw color and min/max properties.
 */
class DrawEditDialog : public wxDialog
{
  public:
	/** Dialog constructor, referenced arguments are modified.
	 * You need to check if min and max have proper values after return
	 * from dialog. Use ShowModal to display dialog, check if return is wxID_OK.
	 * @param col_ind selected color index, -1 for 'auto' color
	 * @param min minimum draw value
	 * @param max maximum draw value
	 */
	DrawEditDialog(wxWindow* parent, int& col_ind, wxString& min, wxString& max);
  protected:
	int& m_col_index;	/**< index of selected color */
#define array_size(a)	(int)(sizeof a / sizeof *a)
	wxToggleButton* m_c[array_size(DrawDefaultColors::dcolors) + 1];
				/**< array of buttons corresponding to colors,
				 * 0-index is 'auto' color */
#undef array_size
	/** Toogle button on/off so they correspond to m_col_index */
	void SetColor();
	/** Called when button is toggled on.
	 *  Toggles other buttons off and modifies m_col_index.
	 */
	void OnColorChanged(wxCommandEvent& ev);
};

#endif
