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

#include <algorithm>

#include <wx/txtstrm.h>

#include "version.h"

#include "classes.h"
#include "drawtb.h"
#include "vercheck.h"
#include "drawapp.h"

BEGIN_EVENT_TABLE(VersionChecker, wxEvtHandler) 
	EVT_TIMER(wxID_ANY, VersionChecker::OnTimer)
	EVT_END_PROCESS(wxID_ANY, VersionChecker::OnTerminate)
END_EVENT_TABLE()
	

VersionChecker* VersionChecker::_version_checker = NULL;
std::vector<DrawToolBar*>* VersionChecker::_toolbars_list = NULL;

VersionChecker::VersionChecker(wxString draw3_path) :
		m_draw3_path(draw3_path), m_process(NULL), m_timer(new wxTimer(this, wxID_ANY))
{
	m_is_new_version = false;

	_version_checker = this;
	_toolbars_list = new std::vector<DrawToolBar*>;
}

void VersionChecker::OnTerminate(wxProcessEvent& e) {
	assert(m_process);

	if (m_process->IsInputAvailable()) {
		wxTextInputStream is(*m_process->GetInputStream());
		wxString version = is.ReadLine();
		if (version != SC::A2S(SZARP_VERSION))
			m_is_new_version = true;
	}

	delete m_process;
	m_process = NULL;

	if (m_is_new_version) {
		ShowNewVersionMessage();
		NotifyNewVersionAvailable();
	} else
		Start();
}

void VersionChecker::Start() {
#ifdef __WXGTK__
	m_timer->Start(1000 * 60 * 30, true);
#endif
}

void VersionChecker::OnTimer(wxTimerEvent& e) {
	m_process = new wxProcess(this, wxID_ANY);
	m_process->Redirect();

	if (wxExecute(m_draw3_path + _T(" -V"), wxEXEC_ASYNC, m_process) == 0) {
		delete m_process;
		m_process = NULL;	
		Start();
	}
}

bool VersionChecker::IsNewVersion() {
	return _version_checker->m_is_new_version;
}

void VersionChecker::AddToolbar(DrawToolBar* toolbar) {
	_toolbars_list->push_back(toolbar);
}

void VersionChecker::RemoveToolbar(DrawToolBar* toolbar) {
	std::vector<DrawToolBar*>::iterator i;
	i = std::remove(_toolbars_list->begin(), _toolbars_list->end(), toolbar);
	_toolbars_list->erase(i, _toolbars_list->end());
}

void VersionChecker::NotifyNewVersionAvailable() {
	for (std::vector<DrawToolBar*>::iterator i = _toolbars_list->begin();
			i != _toolbars_list->end();
			i++) {
		(*i)->NewDrawVersionAvailable();	
	}
}

void VersionChecker::ShowNewVersionMessage() {
	wxMessageBox(_("There is new a version of draw3 installed on this computer, you should restart it."),
					_("New version."),
					wxOK | wxICON_INFORMATION,
					wxGetApp().GetTopWindow());

}

