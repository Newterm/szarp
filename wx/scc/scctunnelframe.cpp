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
 * scc - Szarp Control Center
 * SZARP
 
 *
 * $Id$
 */
#include <libpar.h>
#include "cconv.h"
#include "scctunnelframe.h"
#include "szframe.h"

BEGIN_EVENT_TABLE(SCCTunnelFrame, wxDialog)
    EVT_BUTTON(ID_CloseBtn, SCCTunnelFrame::OnCloseBtn)
    EVT_BUTTON(ID_ConnectBtn, SCCTunnelFrame::OnConnectBtn)
    EVT_CLOSE(SCCTunnelFrame::OnClose)
    EVT_TIMER(SCCTunnelFrame::ID_Timer, SCCTunnelFrame::OnTimer)
END_EVENT_TABLE()

SCCTunnelFrame::SCCTunnelFrame(wxFrame *parent) : wxDialog(parent, ID_TunnelFrame, wxString(_("Support tunnel"))), 
						  connection(NULL), timer(NULL)
{
	SetIcon(szFrame::default_icon);
	wxFlexGridSizer *userpass_sizer = new wxFlexGridSizer(2, 4, 10, 10);

	userpass_sizer->Add(new wxStaticText(this, wxID_ANY, _("Server address:")));
	server_ctrl = new wxTextCtrl(this, wxID_ANY);
	userpass_sizer->Add(server_ctrl);

	userpass_sizer->Add(new wxStaticText(this, wxID_ANY, _("Port number:")));
	port_ctrl = new wxTextCtrl(this, wxID_ANY, _T("9999"));
	userpass_sizer->Add(port_ctrl);
	
	userpass_sizer->Add(new wxStaticText(this, wxID_ANY, _("User:")));
	user_ctrl = new wxTextCtrl(this, wxID_ANY, _T(""));
	userpass_sizer->Add(user_ctrl);
	
	userpass_sizer->Add(new wxStaticText(this, wxID_ANY, _("Password:")));
	password_ctrl = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition,
			wxDefaultSize, wxTE_PASSWORD);
	userpass_sizer->Add(password_ctrl);
	
	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
	connect_btn = new wxButton(this, ID_ConnectBtn, _("Connect"));
	connect_btn->SetDefault();
	button_sizer->Add(connect_btn, 0, wxALIGN_CENTER | wxALL, 10);
	button_sizer->Add(new wxButton(this, ID_CloseBtn, _("Close")),
			0, wxALIGN_CENTER | wxALL, 10);
	
	log = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition,
			wxSize(-1, 300), wxTE_MULTILINE | wxTE_READONLY);

	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(log, 1, wxEXPAND | wxALL, 10);
	sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);
	sizer->Add(userpass_sizer, 0, wxALIGN_CENTER | wxALL, 10);
	sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);
	sizer->Add(button_sizer, 0, wxALIGN_CENTER);

	SetSizer(sizer);
	sizer->SetSizeHints(this);
}

void SCCTunnelFrame::OnCloseBtn(wxCommandEvent& WXUNUSED(event)) {
	Close();
}

void SCCTunnelFrame::OnConnectBtn(wxCommandEvent& WXUNUSED(event)) {

	if (timer == NULL) 
		timer = new wxTimer(this, ID_Timer);
	Connect();
	timer->Start(100);

}

void SCCTunnelFrame::Connect() {

	const wxString& address = server_ctrl->GetValue();
	if (address.length() < 1) {
		wxMessageBox(_("No server address specified"), _("Error"), wxICON_HAND);
		return;
	}

	long port_no;
	if ( !port_ctrl->GetValue().ToLong(&port_no) || port_no < 1024 || port_no > 65535) {
		wxMessageBox(_("Port number must be within range 1024-65535"), _("Error"), wxICON_HAND);
		return;
	}

	connection = new SSHConnection(address, user_ctrl->GetValue().Trim(), password_ctrl->GetValue().Trim(),  port_no);

	if (!connection->Execute()) {
		wxMessageBox(_("Unable to start ssh command"), _("Error"));
		delete connection;
		connection = NULL;
		connect_btn->Enable();
		return;
	}

	connect_btn->Disable();

	log->AppendText(_("Connecting to: "));
	log->AppendText(address);
	log->AppendText(_T("\n"));

}

void SCCTunnelFrame::DeleteConnection() {
	wxSafeYield();
	delete connection;
	connection = NULL;
}
	
void SCCTunnelFrame::OnClose(wxCloseEvent& event) {

	if (timer) 
		timer->Stop();
	if (connection) {
		connection->Terminate();
		DeleteConnection();
	}
	connect_btn->Enable();
	if (event.CanVeto()) {
		event.Veto();
		Hide();
		log->Clear();
	}
	else
		Destroy();
	
}

void SCCTunnelFrame::OnTimer(wxTimerEvent& WXUNUSED(event)) {
	if  (!connection) {
		timer->Stop();
		return;
	}


	SSHConnection::Event event = connection->PollEvent();

	bool failed = false; 	/* true if connection failed*/

	wxString message;
	wxString dialog_message;
	switch (event) {
		case SSHConnection::NONE:
			return;
		case SSHConnection::LISTEN_SUCCESS:
			message = _("Bind to remote port succeeded\nConnection successful to: ");
			message << server_ctrl->GetValue();
			message << _T("\n");
			break;
		case SSHConnection::FAILED_TO_LISTEN:
			dialog_message = message = _("Failed to bind to remote port\nTry another port number\n");
			failed = true;
			break;
		case SSHConnection::NETWORK_DOWN:
			dialog_message = message = _("Network is unreachable\n");
			failed = true;
			break;
		case SSHConnection::NO_ROUTE_TO_HOST:
			dialog_message = message = _("No route to host\n");
			failed = true;
			break;
		case SSHConnection::NEW_TUN_CONNECTION:
			message = _("New connection via channel\n");
			break;
		case SSHConnection::CLOSED_TUN_CONNECTION:
			message = _("Closed channel connection\n");
			break;
		case SSHConnection::CONNECTION_SUCCES:
			message = _("Connected\n");
			break;
		case SSHConnection::PROCESS_KILLED:
			dialog_message = message = _("Unexpected close of the connection\n");
			failed = true;
			break;
		case SSHConnection::NAME_NOT_KNOWN:
			dialog_message = message = _("Address not found\n");
			failed = true;
			break;
		case SSHConnection::TEMP_FAILURE_IN_RESOLV:
			dialog_message = _("Address not found");
			message = _("Possible problem with network configuration\n");
			failed = true;
			break;
		case SSHConnection::AUTH_FAILURE:
			dialog_message = message = _("Authentication failure\n");
			failed = true;
			break;
		case SSHConnection::CONNECTION_REFUSED:
			dialog_message = message = _("Connection refused\n");
			failed = true;
			break;
		case SSHConnection::TIMEOUT:
			dialog_message = message = _("Timeout\n");
			failed = true;
			break;
	}

	log->AppendText(message);
	log->ShowPosition(log->GetLastPosition());

	if (!failed)
		return;

	DeleteConnection();

	wxMessageBox(dialog_message, _("Error"), wxICON_ERROR, this);
	connect_btn->Enable();

}

BEGIN_EVENT_TABLE(SSHConnection, wxEvtHandler)
    EVT_END_PROCESS(wxID_ANY, SSHConnection::OnTerminate)
END_EVENT_TABLE()

SSHConnection::SSHConnection(const wxString& address, const wxString& username, const wxString& password,  long port_no) : 
		process(NULL), pid(-1), event(NONE)
{
	wxSetEnv(_T("SSHPASS"), password);
	command << _T(" sshpass -e ")
		<< _T("ssh -v -N -o StrictHostKeyChecking\\ no ") 
		<< _T("-R ") 
		<< port_no 
		<< _T(":localhost:22 -l ") 
		<< username
		<< _T(" ")
		<< address;

}

const SSHConnection::SSHMessage SSHConnection::SSHMessages[] = {
	{ _T("*Connection established*"), CONNECTION_SUCCES },
	{ _T("*remote forward success*"), LISTEN_SUCCESS },
	{ _T("*port forwarding failed*"), FAILED_TO_LISTEN },
	{ _T("*Network is unreachable*"), NETWORK_DOWN },
	{ _T("*channel*connected*"), NEW_TUN_CONNECTION },
	{ _T("*channel*free*"), CLOSED_TUN_CONNECTION },
	{ _T("*route to host*"), NO_ROUTE_TO_HOST},
	{ _T("*Name*not known*"), NAME_NOT_KNOWN},
	{ _T("*Permission denied*"), AUTH_FAILURE},
	{ _T("*failure in name resolution*"), TEMP_FAILURE_IN_RESOLV},
	{ _T("*Connection refused*"), CONNECTION_REFUSED},
	{ _T("*timed out*"), TIMEOUT},
};

SSHConnection::Event SSHConnection::PollEvent() {

	while (event == NONE && process->IsErrorAvailable()) {
		wxTextInputStream is(*process->GetErrorStream());
		wxString message(is.ReadLine());
		InterpretMessage(message);
	}

	Event ev = this->event;
	this->event = NONE;
	return ev;
}

void SSHConnection::InterpretMessage(const wxString& str) 
{
	const unsigned size = sizeof(SSHMessages)/sizeof(SSHMessage);

	for (unsigned i = 0;i < size;++i) {
		const SSHMessage& msg = SSHMessages[i]; 

		if (!str.Matches(msg.expression))
			continue;

		event = msg.event;
		if (event == FAILED_TO_LISTEN) /*in this case we need to kill ssh process ourselves
						 because ssh won't do that */
				Terminate();
		return;
	}

}

void SSHConnection::Terminate() {
	if (pid == -1)
		return;

	process->SetNextHandler(NULL);
	wxKill(pid);
	pid = -1;
	process = NULL;
}
			
bool SSHConnection::Execute() {

	if (pid != -1)
		Terminate();

	process = new wxProcess(this,wxID_ANY);
	process->Redirect();

	pid = wxExecute(command, wxEXEC_ASYNC, process);

	if (pid == -1) {
		delete process;
		process = NULL;
		return false;
	}

	event = NONE;

	return true;
}

void SSHConnection::OnTerminate(wxProcessEvent& WXUNUSED(evt)) {
	pid = -1;
	Event last_event = PollEvent();
	event = (last_event == NONE) ? PROCESS_KILLED : last_event;

	delete process;
	process = NULL;
}

