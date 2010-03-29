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

#include <wx/process.h>
#include <wx/timer.h>
#include <vector>

class VersionChecker : public wxEvtHandler {
	wxString m_draw3_path;
	wxProcess *m_process;
	wxTimer* m_timer;
	bool m_is_new_version;

	static std::vector<DrawToolBar*>* _toolbars_list;
	static VersionChecker *_version_checker;
public:
	VersionChecker(wxString m_draw3_path);
	void OnTerminate(wxProcessEvent& e);
	void OnTimer(wxTimerEvent& e);
	void Start();
	static bool IsNewVersion(); 
	static void AddToolbar(DrawToolBar* toolbar);
	static void RemoveToolbar(DrawToolBar* toolbar);
	static void ShowNewVersionMessage();
	static void NotifyNewVersionAvailable();
	DECLARE_EVENT_TABLE();
};
