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


#include "dcolors.h"


int DrawDefaultColors::dcolors[][3] = {
	{ 0xFF, 0x00, 	0x00 },
	{ 0xFF, 0xC2, 	0x46 },
	{ 0x00,	0xFF, 	0xFF },
	{ 0x00,	0xFF,	0x00 },
	{ 0xFF,	0xFF,	0x00 },
	{ 0xB9,	0xF4,	0xBE },
	{ 0x00,	0x00,	0xFF },
	{ 0xFF,	0x00,	0xFF },
	{ 0x00,	0xB6, 	0xFF },
	{ 0xA6,	0xA5,	0x50 },
	{ 0xFF,	0x82,	0x5F },
	{ 0xAF, 0xAF, 	0xFF }
};

int DrawDefaultColors::FindIndex(wxColour& col)
{
	for (unsigned i = 0; i < (sizeof dcolors  / sizeof *dcolors); i++) {
		if ((dcolors[i][0] == col.Red()) and (dcolors[i][1] == col.Green())
				and (dcolors[i][2] == col.Blue())) {
			return i;
		}
	}
	return -1;
}

wxColour DrawDefaultColors::MakeColor(int index)
{
	if ((index < 0) or (index >= (int) (sizeof dcolors / sizeof *dcolors))) {
		wxColour ret;
		return ret;
	}
	wxColour ret(dcolors[index][0], dcolors[index][1], dcolors[index][2]);
	return ret;
}

wxString DrawDefaultColors::AsString(int index)
{
	if ((index < 0) or (index >= (int) (sizeof dcolors / sizeof *dcolors))) {
		return wxEmptyString;
	}
	wxColour col = MakeColor(index);
	return col.GetAsString(wxC2S_NAME | wxC2S_HTML_SYNTAX);
}

