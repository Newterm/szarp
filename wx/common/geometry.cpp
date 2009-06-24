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
 * 
 *
 * $Id$
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>

 */

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif


/**
 * Analizes X-Windows style geometry option. Reads requested values for window's
 * positions and size and sets values given as pointers. Values for options that
 * are not set are left unchanged. It is strongly recomended, that at least
 * width and height values should be set before calling functions, because
 * they are used in some cases while setting window's position.
 * @param geometry geometry option string
 * @param x pointer to window's x position value
 * @param y pointer to window's y position value
 * @param width pointer to window's width value
 * @param height pointer to window's height value
 * @return 0 on success, 1 if error occured while parsing options string (even 
 * on error some of the arguments may be affected).
 */ 
int get_geometry(wxString geometry, int* x, int* y, int* width, int* height)
{
	int maxx, maxy;
	wxDisplaySize(&maxx, &maxy);

	int pos = 0;
	int length = geometry.Length();
	int n;
	bool minus, nminus;

	/* get windows' width */
	if ((pos < length) && isdigit(geometry.GetChar(pos))) {
	      	n = 0;
		while ((pos < length) && isdigit(geometry.GetChar(pos))) {
			n = n * 10 + geometry.GetChar(pos) - '0';
			pos++;
		}
		*width = n;
	}

	/* get height */
	if ((pos < length) && (geometry.GetChar(pos) == 'x')) {
		n = 0;
		pos++;
		if ((pos < length) && isdigit(geometry.GetChar(pos))) {
			n = 0;
			while ((pos < length) && isdigit(geometry.GetChar(pos))) {
				n = n * 10 + geometry.GetChar(pos) - '0';
				pos++;
			}
			*height = n;
		}
	}
	
	if (pos >= length)
		return 0;
	/* Set X posisiotn */
	if ((geometry.GetChar(pos) != '+') && (geometry.GetChar(pos) != '-'))
		return 1;
	if (geometry.GetChar(pos) == '-')
		minus = TRUE;
	else
		minus = FALSE;
	if (++pos >= length)
		return 1;
	if (geometry.GetChar(pos) == '-') {
		nminus = true;
		if (++pos >= length)
			return 1;
	} else
		nminus = false;
	n = 0;
	while ((pos < length) && (isdigit(geometry.GetChar(pos)))) {
		n = n * 10 + geometry.GetChar(pos) - '0';
		pos++;
	}
	if (nminus)
		n = -n;
	if (!minus) 
		*x = n;
	else 
		*x = maxx - n - *width;
	if (pos >= length)
		return 0;
	n = 0;
	if (geometry.GetChar(pos) == '-')
		minus = TRUE;
	else
		minus = FALSE;
	if (++pos >= length)
		return 0;
	if (geometry.GetChar(pos) == '-') {
		nminus = true;
		if (++pos >= length)
			return 1;
	} else
		nminus = false;
	while ((pos < length) && (isdigit(geometry.GetChar(pos)))) {
		n = n * 10 + geometry.GetChar(pos) - '0';
		pos++;
	}
	if (nminus)
		n = -n;
	if (!minus) 
		*y = n;
	else 
		*y = maxy - n - *height;
	return 0;
}


