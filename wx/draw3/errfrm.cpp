#include "errfrm.h"
#include "ids.h"
#include "szframe.h"

#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/config.h>
#include <wx/xrc/xmlres.h>
#include <wx/cshelp.h>

#ifndef MINGW32
extern "C" void gtk_window_set_accept_focus(void *window, int setting);
#endif

ErrorFrame *ErrorFrame::_error_frame = NULL;

BEGIN_EVENT_TABLE(ErrorFrame, wxFrame)
	EVT_CLOSE(ErrorFrame::OnClose)
	EVT_BUTTON(XRCID("HIDE_BUTTON"), ErrorFrame::OnHide)
	EVT_BUTTON(XRCID("CLEAR_BUTTON"), ErrorFrame::OnClear)
	EVT_BUTTON(wxID_HELP, ErrorFrame::OnHelpButton)
END_EVENT_TABLE()

void ErrorFrame::Create() {
	if (_error_frame == NULL)
		_error_frame = new ErrorFrame();

#ifndef MINGW32
	gtk_window_set_accept_focus(_error_frame->GetHandle(), 0);
#endif

}

void ErrorFrame::NotifyError(const wxString &s) {
	Create();
	if (s == _error_frame->m_last_message)
		if ((wxDateTime::Now() - _error_frame->m_last_update_time).GetSeconds() < 60)
			return;
	_error_frame->m_last_update_time = wxDateTime::Now();
	_error_frame->m_last_message = s;
	_error_frame->AddErrorText(s);
}


void ErrorFrame::DestroyWindow() {
	if (_error_frame) {
		_error_frame->Destroy();
		_error_frame = NULL;
	}
}

ErrorFrame::ErrorFrame() {
	SetHelpText(_("draw3-ext-errorwindow"));

	wxXmlResource::Get()->LoadFrame(this, NULL, _T("error_frame"));
	SetIcon(szFrame::default_icon);
	SetBackgroundColour(DRAW3_BG_COLOR);

	m_rise_check = XRCCTRL(*this, "RISE_WINDOW_CHECK", wxCheckBox);
	m_text = XRCCTRL(*this, "TEXT_CTRL", wxTextCtrl);

	wxConfigBase* cfg = wxConfigBase::Get();
	bool rise = cfg->Read(_T("rise_error_window"), true);
	m_rise_check->SetValue(rise);
	
}

void ErrorFrame::AddErrorText(const wxString &s) {
	m_text->AppendText(s + _T("\n"));
	m_text->ShowPosition(m_text->GetLastPosition());

	if (m_rise_check->IsChecked()) {
		wxFrame::Show(true);
		Raise();
	}
}

void ErrorFrame::OnHide(wxCommandEvent &event) {
	wxFrame::Show(false);
}

void ErrorFrame::OnClear(wxCommandEvent &event) {
	m_text->Clear();	
}

void ErrorFrame::ShowFrame() {
        Create();
	_error_frame->Show(true);
	_error_frame->Raise();
}

void ErrorFrame::OnClose(wxCloseEvent &event) {
	if (event.CanVeto()) {
		event.Veto();	
		wxFrame::Show(false);
	} else {
		_error_frame = NULL;
		Destroy();
	}
}

void ErrorFrame::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

ErrorFrame::~ErrorFrame() {
	wxConfigBase* cfg = wxConfigBase::Get();
	cfg->Write(_T("rise_error_window"), m_rise_check->IsChecked());
}
