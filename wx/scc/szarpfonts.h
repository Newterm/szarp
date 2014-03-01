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
 * scc - Szarp Control Center - setting fonts for gtk applications and draw
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#ifndef __SZARPFONTS_H__
#define __SZARPFONTS_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif  /* HAVE_CONFIG_H */

#ifndef MINGW32

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif  /* ! WX_PRECOMP */

/** name of directory with szarp configuration */
#define SZARPDIR _T(".szarp")

/** 
 * Dialog for selecting default font size for almost all SZARP applications.
 * For every type of application we have to do different things:
 * - for wx (Gtk) applications we create ~/.szarp/gtk.rc file with Gtk
 *   resource definitions; this file is included by applications using szApp
 *   class
 * - for Raporter Tcl/Tk [deleted]
 * - for SzarpDraw (Motif) [deleted]
 */
class szCommonFontDlg : public wxDialog {
public:
	szCommonFontDlg(wxWindow *parent);
	/** Event handler - changes label font size when selecting size from
	 * choice list. */
	void OnChoice(wxCommandEvent& event);
	/** Event handler - sets font size to selected value. Skips
	 * event to perform default action for ID_OK button (close window
	 * with OK status). */
	void OnApply(wxCommandEvent& event);
protected:
	/** Create directory with given name in users home directory. Do
	 * nothing if directory exists.
	 * @return wxEmptyString on error, directory name if directory was 
	 * successfully created or already exists
	 */
	wxString CreateSzarpDir();
	/** Create gtk.rc file for wxGtk applications. 
	 * @param dir name of directory where to create file */
	void CreateGtkConfig(wxString dir);

	/** minimum point font size */
	static const int min_font_size = 8;
	/** maximum point font size */
	static const int max_font_size = 24;
	/** label for presenting selected font size */
	wxStaticText* m_text;
	/** Main window sizer. Pointer used to resize window after font 
	 * size change. */
	wxSizer* main_s;
	/** Selected font size. */
	int m_fsize;
	
	DECLARE_EVENT_TABLE()
};

#endif  /* ! MINGW32 */

#endif  /* __SZARP_FONTS_H__ */

