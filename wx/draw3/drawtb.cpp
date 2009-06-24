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
 
 * Micha³ Blajerski <nameless@praterm.com.pl>
 *
 * $Id$
 * Mini toolbar to control menubar and most used draw3 functions.
 */


#include <wx/config.h>

#include "drawtb.h"
#include "drawfrm.h"
#include "bitmaps/help.xpm"
#include "bitmaps/exit.xpm"
#include "bitmaps/find.xpm"
#include "bitmaps/plus.xpm"
#include "bitmaps/split.xpm"
#include "bitmaps/filter0.xpm"
#include "bitmaps/filter1.xpm"
#include "bitmaps/filter2.xpm"
#include "bitmaps/filter3.xpm"
#include "bitmaps/filter4.xpm"
#include "bitmaps/filter5.xpm"
#include "bitmaps/refresh.xpm"
#include "bitmaps/flag_fr.xpm"
#include "bitmaps/flag_pl.xpm"
#include "bitmaps/flag_gb.xpm"
#include "bitmaps/flag_de.xpm"
#include "bitmaps/flag_sr.xpm"
#include "bitmaps/flag_hu.xpm"
#include "bitmaps/flag_it.xpm"

DrawToolBar::DrawToolBar(wxWindow *parent) :
	wxToolBar(parent,-1)
{
	SetHelpText(_T("draw3-toolbar-items"));

	AddTool(drawTB_ABOUT, _T(""),help_xpm,_("About"));
	AddTool(drawTB_FIND, _T(""),find_xpm, _("Find"));
	AddTool(drawTB_SUMWIN, _T(""),plus_xpm, _("Summary Window"));
	AddTool(drawTB_SPLTCRS, _T(""),split_xpm, _("Split cursor"), wxITEM_CHECK);
	AddTool(drawTB_FILTER, _T(""),filter0_xpm, _("Filter"));
	AddTool(drawTB_REFRESH, _T(""),refresh_xpm, _("Refresh"));

	wxString lang = wxConfig::Get()->Read(_T("LANGUAGE"), _T("pl"));
	const char **lang_icon;

	if (lang == _T("pl"))
		lang_icon = flag_pl;
	else if (lang == _T("en"))
		lang_icon = flag_gb;
	else if (lang == _T("de"))
		lang_icon = flag_de;
	else if (lang == _T("sr"))
		lang_icon = flag_sr;
	else if (lang == _T("it"))
		lang_icon = flag_it;
	else if (lang == _T("hu"))
		lang_icon = flag_hu;
	else
		lang_icon = flag_fr;

	AddTool(drawTB_LANGUAGE, _T(""), lang_icon, _("Language"));
	AddTool(drawTB_EXIT, _T(""),exit_xpm, _("Quit"));
	Realize();
}

void DrawToolBar::DoubleCursorToolCheck() {
	ToggleTool(drawTB_SPLTCRS, true);
}

void DrawToolBar::DoubleCursorToolUncheck() {
	ToggleTool(drawTB_SPLTCRS, false);
}

void DrawToolBar::SetFilterToolIcon(int i) {
	char const **tool_icon;

	switch (i) {
		case 0:
			tool_icon = filter0_xpm;
			break;
		case 1:
			tool_icon = filter1_xpm;
			break;
		case 2:
			tool_icon = filter2_xpm;
			break;
		case 3:
			tool_icon = filter3_xpm;
			break;
		case 4:
			tool_icon = filter4_xpm;
			break;
		case 5:
			tool_icon = filter5_xpm;
			break;
		default:
			assert(false);
	}
	int index = GetToolPos(drawTB_FILTER);
	assert(index != wxNOT_FOUND);

	DeleteTool(drawTB_FILTER);
	InsertTool(index, drawTB_FILTER, tool_icon, wxNullBitmap, false, NULL, _T(""), _("Filter"));
	Realize();
}

