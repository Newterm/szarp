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

BEGIN_EVENT_TABLE(SCCTunnelFrame, wxDialog)
    EVT_BUTTON(ID_CloseBtn, SCCTunnelFrame::OnCloseBtn)
    EVT_BUTTON(ID_ConnectBtn, SCCTunnelFrame::OnConnectBtn)
    EVT_CLOSE(SCCTunnelFrame::OnClose)
    EVT_TIMER(SCCTunnelFrame::ID_Timer, SCCTunnelFrame::OnTimer)
END_EVENT_TABLE()

SCCTunnelFrame::SCCTunnelFrame(wxFrame *parent) : wxDialog(parent, ID_TunnelFrame, wxString(_("Support tunnel"))), 
						  connection(NULL), timer(NULL), current_address(0)
{
	wxBoxSizer *userpass_sizer = new wxBoxSizer(wxHORIZONTAL);
	userpass_sizer->Add(new wxStaticText(this, wxID_ANY, _("User:")),
			0, wxALIGN_CENTER | wxALL, 10);
	
	user_ctrl = new wxTextCtrl(this, wxID_ANY, _T(""));
	userpass_sizer->Add(user_ctrl, 0, wxALIGN_CENTER | wxALL, 10);
	
	userpass_sizer->Add(new wxStaticText(this, wxID_ANY, _("Password:")),
			0, wxALIGN_CENTER | wxALL, 10);

	password_ctrl = new wxTextCtrl(this, wxID_ANY, _T(""));
	userpass_sizer->Add(password_ctrl, 0, wxALIGN_LEFT | wxALL, 10);
	
	wxBoxSizer *port_sizer = new wxBoxSizer(wxHORIZONTAL);
	port_sizer->Add(new wxStaticText(this, wxID_ANY, _("Port number:")),
			0, wxALIGN_CENTER | wxALL, 10);
	
	port_ctrl = new wxTextCtrl(this, wxID_ANY, _T("9999"));
	port_sizer->Add(port_ctrl, 0, wxALIGN_CENTER | wxALL, 10);
	
	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
	connect_btn = new wxButton(this, ID_ConnectBtn, _("Connect"));
	button_sizer->Add(connect_btn, 0, wxALIGN_CENTER | wxALL, 10);
	button_sizer->Add(new wxButton(this, ID_CloseBtn, _("Close")),
			0, wxALIGN_CENTER | wxALL, 10);
	
	log = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition,
			wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(log, 1, wxEXPAND);
	sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);
	sizer->Add(userpass_sizer, 0, wxALIGN_CENTER);
	sizer->Add(port_sizer, 0, wxALIGN_CENTER);
	sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);
	sizer->Add(button_sizer, 0, wxALIGN_CENTER);

	SetSizer(sizer);

	char *tunnel_addresses = libpar_getpar("scc", "tunnel_endpoints", 0);
	if (!tunnel_addresses) {
		addresses.push_back(wxString(_T("praterm")));
		addresses.push_back(wxString(_T("praterm.com.pl")));
	} else {
		wxStringTokenizer tok = wxStringTokenizer(wxString(SC::A2S(tunnel_addresses)), _T(","));
		while (tok.HasMoreTokens()) 
			addresses.push_back(tok.GetNextToken().Trim());	
		free(tunnel_addresses);
	}

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

	if (!addresses[current_address].valid) {
		for (current_address = 0; current_address < addresses.size(); ++current_address)
			if (addresses[current_address].valid) 
				break;
		//if all addresses are invalid - mark all of them as valid so later
		//we can try again, but do not connect now
		if (current_address == addresses.size()) {
			for (vector<address>::size_type i = 0; i < addresses.size(); ++i)
				addresses[i].valid = true;
			current_address = 0;
			connect_btn->Enable();
			timer->Stop();
			wxMessageBox(_("Unable to connect"), _("Error"));
			return;
		}
		
	}

	const wxString& address = addresses[current_address].addr;

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
	bool try_other = true;  /* in case of failure indicates if we shall
				   try another address from list*/

	wxString message;
	wxString dialog_message;
	switch (event) {
		case SSHConnection::NONE:
			return;
		case SSHConnection::LISTEN_SUCCESS:
			message = _("Bind to remote port succeeded\nConnection successful to: ");
			message << addresses[current_address].addr;
			message << _T("\n");
			break;
		case SSHConnection::FAILED_TO_LISTEN:
			dialog_message = message = _("Failed to bind to remote port\nTry another port number\n");
			failed = true;
			try_other = false;
			break;
		case SSHConnection::NETWORK_DOWN:
			dialog_message = message = _("Network is unreachable\n");
			failed = true;
			try_other = false;
			break;
		case SSHConnection::NO_ROUTE_TO_HOST:
			message = _("No route to host\n");
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
			try_other = false;
			break;
		case SSHConnection::NAME_NOT_KNOWN:
			message = _("Address not found\n");
			failed = true;
			break;
		case SSHConnection::TEMP_FAILURE_IN_RESOLV:
			dialog_message = _("Address not found");
			message = _("Possible problem with network configuration\n");
			failed = true;
			try_other = false;
			break;
		case SSHConnection::AUTH_FAILURE:
			message = _("Authentication failure");
			failed = true;
			try_other = true;
			break;
		case SSHConnection::CONNECTION_REFUSED:
			message = _("Connection refused\n");
			failed = true;
			try_other = true;
			break;
		case SSHConnection::TIMEOUT:
			message = _("Timeout\n");
			failed = true;
			try_other = true;
			break;
	}

	log->AppendText(message);
	log->ShowPosition(log->GetLastPosition());

	if (!failed)
		return;

	DeleteConnection();

	if (try_other) {
		addresses[current_address].valid = false;
		Connect();
		return;
	}
	else {
		wxMessageBox(dialog_message, _("Error"));
		connect_btn->Enable();
	}

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

