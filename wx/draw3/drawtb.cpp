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
 
 * Michał Blajerski <nameless@praterm.com.pl>
 *
 * $Id$
 * Mini toolbar to control menubar and most used draw3 functions.
 */


#include <wx/config.h>

#include "ids.h"
#include "drawtb.h"
#include "vercheck.h"
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
#include "bitmaps/remark.xpm"
#include "bitmaps/flag_checkered.xpm"
#include "bitmaps/oldverwarn.xpm"
#include "bitmaps/draw_tree.xpm"
#include "bitmaps/florence.xpm"

#ifndef MINGW32
std::string exec_cmd (const char* cmd)
{
	char buffer[128];
	std::string result = "";
	FILE* pipe = popen(cmd, "r");

	if (!pipe)
		return "";

	while (!feof(pipe)) {
		if (NULL != fgets(buffer, 128, pipe))
			result += buffer;
	}

	pclose(pipe);
	return result;
}
#endif

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
	AddTool(drawTB_DRAWTREE, _T(""),draw_tree_xpm, _("Tree Set"));
/** disable florence keyboard in windows builds */
#ifndef MINGW32
	AddTool(drawTB_FLORENCE, _T(""), florence_xpm, _("Show screen keyboard"));
#endif
	AddTool(drawTB_GOTOLATESTDATE, _T(""), flag_checkered_xpm, _("Go to latest date"));
	AddTool(drawTB_REMARK, _T(""), remark_xpm, _("Remarks"));
	AddTool(drawTB_EXIT, _T(""),exit_xpm, _("Quit"));
	if (VersionChecker::IsNewVersion())
		NewDrawVersionAvailable();
	else
		Realize();

#ifndef MINGW32
	/* disable florence button if program is not present */
	std::string path = exec_cmd("which florence");
	path = path.substr(0, path.size()-1);

	if (access(path.c_str(), X_OK))
		EnableTool(drawTB_FLORENCE, false);
#endif

	VersionChecker::AddToolbar(this);
}

void DrawToolBar::DoubleCursorToolCheck() {
	ToggleTool(drawTB_SPLTCRS, true);
}

void DrawToolBar::DoubleCursorToolUncheck() {
	ToggleTool(drawTB_SPLTCRS, false);
}

void DrawToolBar::NewDrawVersionAvailable() {
	int index = GetToolPos(drawTB_EXIT);
	InsertTool(index, drawTB_NEWDRAWVERSION, old_version_warning_xpm, wxNullBitmap, false, NULL, _T(""), _("New version available"));
	Realize();
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

void DrawToolBar::NewVersionToolClicked(wxCommandEvent &e) {
	VersionChecker::ShowNewVersionMessage();
}

DrawToolBar::~DrawToolBar() {
	VersionChecker::RemoveToolbar(this);
}

BEGIN_EVENT_TABLE(DrawToolBar, wxToolBar)
	LOG_EVT_MENU(drawTB_NEWDRAWVERSION, DrawToolBar , NewVersionToolClicked, "drawtb:newdrawversion" )
END_EVENT_TABLE()
