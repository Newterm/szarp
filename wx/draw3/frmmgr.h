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
 
 *
 * $Id$
 */

#ifndef __FRMMGR_H__
#define __FRMMGR_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/datetime.h>
#endif

#include <vector>

/**class responsible for creating and managing @see DrawFrame objects*/
class FrameManager : public wxEvtHandler {
	WX_DEFINE_ARRAY(DrawFrame*, DrawFramePtrArray);

	/**array of @see DrawFrames*/
	DrawFramePtrArray frames;

	/**@see DatabaseManager*/
	DatabaseManager *database_manager;

	/**@see ConfigManager*/
	ConfigManager *config_manager;

	/**@see RemarksHandler*/
	RemarksHandler *remarks_handler;

	/**@see StatDialog*/
	StatDialog *stat_dialog;

	/**@see RemarksFrame*/
	RemarksFrame *remarks_frame;

	/**identifier that will be assigned to next created frame*/
	int free_frame_number;

	public:
	FrameManager(DatabaseManager *pmgr, ConfigManager *cfgmgr, RemarksHandler* rhandle);
	/**creates new frame with given config
	 * @param prefix of base to use
	 * @param window name of set to start with, empty string if first set shall be used
	 * @param time time to start with
	 * @param size initial size of frame
	 * @param position initial position of frame
	 * @return true if config was successfully read and frame created*/
	bool CreateFrame(const wxString &prefix, const wxString& set, PeriodType pt, time_t time, const wxSize& size, const wxPoint &position, int selected_draw = 0, bool try_load_layout = false);


	/**postition current panel at following params
	 * @param prefix of base to use
	 * @param window name of set 
	 * @param time time to start with
	 * @param size initial size of frame
	 * @param position initial position of frame
	 * @return true if config was successfully read and frame created*/
	bool OpenInExistingFrame(const wxString &prefix, const wxString& set, PeriodType pt, time_t time, int selected_draw);

	/**Close event hadnler, asks user for and confirmation wheather frame shall be really closed.
	 * If closed frame is the only frame application is closed*/
	void OnClose(wxCloseEvent &event);

	/**Pops up dialog for choosing xy graph parameters
	 * @param prefix initial configutation prefix
	 * @param time information about time set in current draw
	 * @param users_draws list of selected draws by user */
	void CreateXYGraph(wxString prefix, TimeInfo time, std::vector<DrawInfo*> user_draws);

	void CreateXYZGraph(wxString prefix, TimeInfo time, std::vector<DrawInfo*> user_draws);

	/**Shows dialog statistics calculation
	 * @param prefix initial configuration prefix
	 * @param time information about time set in current draw
	 * @param user_draws list of selected draws by user */
	void ShowStatDialog(wxString prefix, TimeInfo time, std::vector<DrawInfo*> user_draws);

	/**Find @see DrawFrame with given numer
	 * @param number number of @see DrawFrame to find
	 * @returns pointer to a frame with given numer or NULL if no such frame exists*/
	DrawFrame* FindFrame(int number);

	void LoadConfig(DrawFrame *frame);

	DECLARE_EVENT_TABLE()
};

#endif	
