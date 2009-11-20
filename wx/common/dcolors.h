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
 * SZARP
 *
 * $Id$
 */


#ifndef __DCOLORS_H__
#define __DCOLORS_H__

#include <wx/colour.h>

class DrawDefaultColors {
  public:
	static int dcolors[12][3];
	/** Return index of color col in dcolors table, -1 if not found */
	static int FindIndex(wxColour& col);
	/** Return string representation of color, wxEmptyString for color not in dcolor
	 * @param index index of color in dcolors table
	 */
	static wxString AsString(int index);
	/** Return wxColour object for colour with given index. */
	static wxColour MakeColor(int index);
};


/* list of default draw colors */


#endif
