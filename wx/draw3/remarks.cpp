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

/* Remarks handling code 
 *
 * $Id$
 */

#include "config.h"

#include <wx/config.h>
#include <wx/filename.h>
#include <libxml/tree.h>
#include <wx/xrc/xmlres.h>

#ifndef MINGW32
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <sys/types.h>
#include <shlwapi.h>
#endif

#include <ares.h>
#include <ares_dns.h>

#include <vector>
#include <sstream>
#include <iostream>
#include <functional>
#include <ios>

#include <boost/make_shared.hpp>

#include <wx/file.h>
#include <wx/dir.h>
#include <wx/config.h>
#include <wx/tokenzr.h>

#include "szframe.h"
#include "szhlpctrl.h"
#include "xmlutils.h"

#include "ids.h"
#include "classes.h"
#include "sprecivedevnt.h"
#include "parameditctrl.h"
#include "seteditctrl.h"
#include "drawapp.h"
#include "drawobs.h"
#include "drawtime.h"
#include "coobs.h"
#include "cfgmgr.h"
#include "cconv.h"
#include "version.h"
#include "dbinquirer.h"
#include "drawfrm.h"
#include "drawpnl.h"
#include "dbinquirer.h"
#include "database.h"
#include "draw.h"
#include "errfrm.h"
#include "md5.h"
#include "drawsctrl.h"
#define REMARKS_CPP
#include "remarks.h"
#include "defcfg.h"

#include "timeformat.h"

void debug_print_long(const wchar_t *l);

DEFINE_EVENT_TYPE(REMARKSSTORED_EVENT)

typedef void (wxEvtHandler::*RemarksStoredEventFunction)(RemarksStoredEvent&);

#define EVT_REMARKS_STORED(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( REMARKSSTORED_EVENT, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) \
    wxStaticCastEvent( RemarksStoredEventFunction, & fn ), (wxObject *) NULL ),

RemarksStoredEvent::RemarksStoredEvent(std::vector<Remark> remarks, time_t fetch_time, bool manually_triggered)
		: wxCommandEvent(REMARKSSTORED_EVENT, wxID_ANY) {
	m_remarks = remarks;
	m_fetch_time = fetch_time;
	m_manually_triggered = manually_triggered;
}

std::vector<Remark>& RemarksStoredEvent::GetRemarks() {
	return m_remarks;
}

time_t RemarksStoredEvent::GetFetchTime() {
	return m_fetch_time;
}

bool RemarksStoredEvent::GetManuallyTriggered() {
	return m_manually_triggered;
}

wxEvent* RemarksStoredEvent::Clone() const {
	return new RemarksStoredEvent(*this);
}

const int REMARKS_SERVER_PORT = 7998;

RemarksHandler::RemarksHandler(ConfigManager *config_manager) : m_config_manager(config_manager) {

	wxConfigBase* config = wxConfig::Get();

	long first_run_with_remarks = 1L;

	config->Read(_T("FIRST_RUN_WITH_REMARKS"), &first_run_with_remarks);
	config->Read(_T("REMARKS_SERVER_ADDRESS"), &m_server);
	config->Read(_T("REMARKS_SERVER_USERNAME"), &m_username);
	config->Read(_T("REMARKS_SERVER_PASSWORD"), &m_password);
	config->Read(_T("REMARKS_SERVER_AUTOFETCH"), &m_auto_fetch, false);

	m_configured = !m_server.IsEmpty() && !m_username.IsEmpty() && !m_password.IsEmpty();

	if (!m_configured && first_run_with_remarks) {
		GetConfigurationFromSSCConfig();

		m_configured = !m_server.IsEmpty() && !m_username.IsEmpty() && !m_password.IsEmpty();

		config->Write(_T("REMARKS_SERVER_ADDRESS"), m_server);
		config->Write(_T("REMARKS_SERVER_USERNAME"), m_username);
		config->Write(_T("REMARKS_SERVER_PASSWORD"), m_password);
		config->Write(_T("FIRST_RUN_WITH_REMARKS"), 0L);

		config->Flush();
	}

	m_timer.SetOwner(this);
	if (m_auto_fetch)
		m_timer.Start(1000 * 60 * 10);

	m_storage = new RemarksStorage(this);

	m_connection = NULL;

}

void RemarksHandler::GetConfigurationFromSSCConfig() {
	wxConfig* ssc_config = new wxConfig(_T("ssc"));

	wxString servers, server;

	ssc_config->Read(_T("SERVERS"), &servers);

	wxStringTokenizer tkz(servers, _T(";"), wxTOKEN_STRTOK);
	if (!tkz.HasMoreTokens()) {
		delete ssc_config;
		return;
	}

	server = tkz.GetNextToken();

	ssc_config->Read(server + _T("/USERNAME"), &m_username);
	ssc_config->Read(server + _T("/PASSWORD"), &m_password);
	m_server = server;

	delete ssc_config;

}

bool RemarksHandler::Configured() {
	return m_configured;
}

void RemarksHandler::OnTimer(wxTimerEvent &event) {
	if (m_configured && m_auto_fetch) {
		GetConnection()->FetchNewRemarks(false);
	} else
		m_timer.Stop();
}

void RemarksHandler::AddRemarkReceiver(RemarksReceiver* receiver) {
	m_receivers.push_back(receiver);
}

void RemarksHandler::RemoveRemarkReceiver(RemarksReceiver* receiver) {
	std::vector<RemarksReceiver*>::iterator i = std::remove(m_receivers.begin(), m_receivers.end(), receiver);
	m_receivers.erase(i, m_receivers.end());
}

void RemarksHandler::NewRemarks(std::vector<Remark>& remarks, time_t fetch_time, bool manually_triggered) {
	m_storage->StoreRemarks(remarks, fetch_time, manually_triggered);
}

void RemarksHandler::OnRemarksStored(RemarksStoredEvent &event) {
	std::vector<Remark>& remarks = event.GetRemarks();

	for (std::vector<RemarksReceiver*>::iterator i = m_receivers.begin();
			i != m_receivers.end();
			i++)
		(*i)->RemarksReceived(remarks);


	if (remarks.size()) {
		wxConfig::Get()->Write(_T("RemarksLatestRetrieval"), long(event.GetFetchTime()));
		wxConfig::Get()->Flush();
	}

	if (event.GetManuallyTriggered()) {
		if (remarks.size())
			wxMessageBox(_("New remarks received"), _("New remarks"), wxICON_INFORMATION);
		else
			wxMessageBox(_("No new remarks"), _("Remarks"), wxICON_INFORMATION);
	}
}

void RemarksHandler::Start() {
	m_storage->Create();
	m_storage->SetPriority(WXTHREAD_MAX_PRIORITY);
	m_storage->Run();
}

void RemarksHandler::Stop() {
	m_storage->Finish();
	m_storage->Wait();
}

RemarksConnection* RemarksHandler::GetConnection() {

	if (m_connection == NULL) {
		m_connection = new RemarksConnection(m_server, this, m_config_manager);
		m_connection->SetUsernamePassword(m_username, m_password);
	}

	return m_connection;

}

RemarksStorage* RemarksHandler::GetStorage() {
	return m_storage;
}

void RemarksHandler::GetConfiguration(wxString& username, wxString& password, wxString &server, bool& autofetch) {
	username = m_username;
	password = m_password;
	server = m_server;
	autofetch = m_auto_fetch;
}

wxString RemarksHandler::GetUsername() const {
	return m_username;
}

void RemarksHandler::SetConfiguration(wxString username, wxString password, wxString server, bool autofetch) {
	m_username = username;
	if (password.IsEmpty() == false)
		m_password = sz_md5(password);

	m_server = server;

	m_configured = !m_server.IsEmpty() && !m_username.IsEmpty() && !m_password.IsEmpty();

	if (m_configured && m_connection) {
		m_connection->SetIPAddress(m_server);
		m_connection->SetUsernamePassword(m_username, m_password);
	}

	if (!m_auto_fetch && autofetch)
		m_timer.Start(1000 * 60 * 10);

	m_auto_fetch = autofetch;

	wxConfigBase* config = wxConfig::Get();

	config->Write(_T("REMARKS_SERVER_ADDRESS"), m_server);
	config->Write(_T("REMARKS_SERVER_USERNAME"), m_username);
	config->Write(_T("REMARKS_SERVER_PASSWORD"), m_password);
	config->Write(_T("REMARKS_SERVER_AUTOFETCH"), m_auto_fetch);

	config->Flush();
}

std::set<std::wstring> RemarksHandler::GetPrefixes() {
	return m_storage->GetPrefixes();
}

BEGIN_EVENT_TABLE(RemarksHandler, wxEvtHandler) 
	EVT_TIMER(wxID_ANY, RemarksHandler::OnTimer)
	EVT_REMARKS_STORED(wxID_ANY, RemarksHandler::OnRemarksStored)
END_EVENT_TABLE()

DECLARE_EVENT_TYPE(XMLRPCREQUEST_EVENT, -1)
DEFINE_EVENT_TYPE(XMLRPCREQUEST_EVENT)

typedef void (wxEvtHandler::*XMLRPCRequestEventFunction)(XMLRPCResponseEvent&);

#define EVT_XMLRPCREQUEST(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( XMLRPCREQUEST_EVENT, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) \
    wxStaticCastEvent( XMLRPCRequestEventFunction, & fn ), (wxObject *) NULL ),


XMLRPCResponseEvent::XMLRPCResponseEvent(XMLRPC_REQUEST request) : wxCommandEvent(XMLRPCREQUEST_EVENT, wxID_ANY) {
	m_request = boost::shared_ptr<_xmlrpc_request>(request, std::bind2nd(std::ptr_fun(XMLRPC_RequestFree), 1));
}

boost::shared_ptr<_xmlrpc_request> XMLRPCResponseEvent::GetRequest() {
	return m_request;
}

wxEvent* XMLRPCResponseEvent::Clone() const {
	return new XMLRPCResponseEvent(*this);
}

XMLRPCConnection::XMLRPCConnection(wxString& address, wxEvtHandler *event_handler) {
	m_read_state = CLOSED;

	m_socket = NULL;

	m_read_buf = NULL;

	m_read_buf_read_pos = 0;

	m_read_buf_write_pos = 0;

	m_read_buf_size = 0;
	
	m_handler = event_handler;

	wxIPV4address ipaddress;
	ipaddress.Service(REMARKS_SERVER_PORT);
	ipaddress.Hostname(address);

	m_socket = new SSLSocketConnection(ipaddress, this);
}

void XMLRPCConnection::SetIPAddress(const wxString &ip) {

	wxIPV4address address;
	address.Service(REMARKS_SERVER_PORT);
	address.Hostname(ip);
	m_socket->SetIPAddress(address);
}

void XMLRPCConnection::ReturnError(std::wstring error) {
	XMLRPC_REQUEST request = XMLRPC_RequestNew();
	XMLRPC_VALUE fault = XMLRPC_UtilityCreateFault(0, (const char *)SC::S2U(error).c_str());
	XMLRPC_RequestSetData(request, fault); 

	XMLRPC_RequestSetRequestType(request, xmlrpc_request_response);

	XMLRPCResponseEvent rev(request);
	wxPostEvent(m_handler, rev);
}

void XMLRPCConnection::ReturnResponse() {
	XMLRPC_REQUEST request = XMLRPC_REQUEST_FromXML(m_read_buf + m_read_buf_read_pos, m_read_buf_write_pos - m_read_buf_read_pos, NULL);

	//fprintf(stdout, "%.*s\n", m_read_buf_write_pos - m_read_buf_read_pos,  m_read_buf + m_read_buf_read_pos);

	XMLRPCResponseEvent rev(request);
	wxPostEvent(m_handler, rev);

}

void XMLRPCConnection::WriteData() {
	while (m_write_buf_pos < m_write_buf.size()) {
		ssize_t written = m_socket->Write(const_cast<char*>(m_write_buf.c_str() + m_write_buf_pos),
					m_write_buf.size() - m_write_buf_pos);

		if (written <= 0) {
			if (m_socket->LastError() != wxSOCKET_WOULDBLOCK) {
				m_socket->Disconnect();
				m_read_state = CLOSED;
				ReturnError(L"Server connection failure");
			}
			break;
		}

		m_write_buf_pos += written;
	}

}

bool XMLRPCConnection::GetLine(std::string& line) {

	for (size_t i = m_read_buf_read_pos; i < m_read_buf_write_pos; i++) {
		if (m_read_buf[i] == '\r'
				&& (i + 1) < m_read_buf_write_pos
				&& m_read_buf[i + 1] == '\n') {
			m_read_buf[i] = '\0';

			line = m_read_buf + m_read_buf_read_pos;
			m_read_buf_read_pos = i + 2;

			return true;
		}

	}

	return false;

}

void XMLRPCConnection::ReadFirstLine(bool& more_data) {
	std::string line;

	more_data = false;

	if (!GetLine(line)) {
		more_data = true;
		return;
	}

	try { 
		std::istringstream iss(line);
		iss.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
		iss.ignore(1000, ' ');

		int status_code;
		iss >> status_code;
		if (status_code != 200) {
			m_read_state = CLOSED;
			m_socket->Disconnect();

			ReturnError(L"Invalid response from server");
		} else {
			m_read_state = READING_HEADERS;
		}
	} catch (std::ios_base::failure) {
		m_read_state = CLOSED;
		m_socket->Disconnect();

		ReturnError(L"Invalid response from server");
		more_data = false;
	}
}

void XMLRPCConnection::ReadHeaders(bool& more_data) {
	std::string line;

	try {

		while (GetLine(line)) {
			if (line == "") {
				m_read_state = READING_CONTENT; 
				more_data = false;
				return ;
			}

			std::istringstream iss(line);
			iss.exceptions(std::ios_base::failbit | std::ios_base::badbit);
			std::string field_name;
			iss >> field_name; 
			if (!strcasecmp(field_name.c_str(), "Content-length:"))
				iss >> m_content_length;
		}

	} catch (std::ios_base::failure) {
		m_read_state = CLOSED;
		m_socket->Disconnect();

		ReturnError(L"Invalid response from server");

		more_data = false;
		return;
	}

	more_data = true;

}

void XMLRPCConnection::ReadContent(bool& more_data) {
	if (m_read_buf_write_pos - m_read_buf_read_pos == m_content_length) {
		m_read_state = CLOSED;
		m_socket->Disconnect();

		more_data = false;

		ReturnResponse();
	} else {
		more_data = true;
	}

}
	
void XMLRPCConnection::ProcessReadData() {
	if (m_read_state == CLOSED)
		return;

	bool more_data_needed;
	std::string line;
	do {
		switch (m_read_state) {
			case READING_FIRST_LINE:
				ReadFirstLine(more_data_needed);
				break;
			case READING_HEADERS:
				ReadHeaders(more_data_needed);
				break;
			case READING_CONTENT:
				ReadContent(more_data_needed);
				break;
			case CLOSED:
				return;
		}
	} while (!more_data_needed);

}

void XMLRPCConnection::ReadData() {

	while (m_read_state != CLOSED) {
		if (m_read_buf_write_pos == m_read_buf_size) {
			if (m_read_buf_size) 
				m_read_buf_size *= 2;
			else 
				m_read_buf_size = 4096;

			m_read_buf = (char*) realloc(m_read_buf, m_read_buf_size);
		}

		ssize_t ret = m_socket->Read(m_read_buf + m_read_buf_write_pos, m_read_buf_size - m_read_buf_write_pos);

		if (ret <= 0) {
			if (m_socket->LastError() != wxSOCKET_WOULDBLOCK) {
				m_socket->Disconnect();
				m_read_state = CLOSED;
				ReturnError(L"Server communication error");
			}
			break;
		}

		m_read_buf_write_pos += ret;

		ProcessReadData();

	}

}

void XMLRPCConnection::OnSocketEvent(wxSocketEvent &event) {
	wxSocketNotify type = event.GetSocketEvent();

	switch (type) {
		case wxSOCKET_CONNECTION:
			WriteData();
			break;

		case wxSOCKET_LOST:
			if (m_read_state != CLOSED) {
				m_socket->Disconnect();
				m_read_state = CLOSED;
				ReturnError(L"Server connection failure");
			}
			break;
		case wxSOCKET_INPUT:
			ReadData();
			break;
		case wxSOCKET_OUTPUT:
			WriteData();
			break;
	}

}

void XMLRPCConnection::PostRequest(XMLRPC_REQUEST request) {

	if (m_read_state != CLOSED) {
		assert(m_read_state == CLOSED);
	}

	m_write_buf_pos = 0;
	m_write_buf = CreateHTTPRequest(request);

	free(m_read_buf);
	m_read_buf = NULL;
	m_read_buf_write_pos 
		= m_read_buf_read_pos 
		= m_read_buf_size = 0;

	m_read_state = READING_FIRST_LINE;

	m_socket->Connect();
}

BEGIN_EVENT_TABLE(XMLRPCConnection, wxEvtHandler)
	EVT_SOCKET(wxID_ANY, XMLRPCConnection::OnSocketEvent)
END_EVENT_TABLE()


XMLRPCConnection::~XMLRPCConnection() {
	free(m_read_buf);

	delete m_socket;
}

std::string XMLRPCConnection::CreateHTTPRequest(XMLRPC_REQUEST request) {
	int len;
	char *request_string = XMLRPC_REQUEST_ToXML(request, &len);

	std::stringstream ss;
	ss << "POST /REMARKS HTTP/1.0\r\n";
	ss << "User Agent: DRAW3/" << SZARP_VERSION << "\r\n";
	ss << "Host: DRAW3(" << SZARP_VERSION << ")\r\n";
	ss << "Content-Type: text/xml\r\n";
	ss << "Content-Length: " << len << "\r\n";
	ss << "\r\n";
	ss << request_string;

	free(request_string);

	return ss.str();

}

DECLARE_EVENT_TYPE(DNSRESPONSE_EVENT, -1)
DEFINE_EVENT_TYPE(DNSRESPONSE_EVENT)

DNSResponseEvent::DNSResponseEvent(std::wstring address) : wxCommandEvent(DNSRESPONSE_EVENT, wxID_ANY), m_address(address) {}

std::wstring DNSResponseEvent::GetAddress() {
	return m_address;
}

wxEvent* DNSResponseEvent::Clone() const {
	return new DNSResponseEvent(*this);
}


typedef void (wxEvtHandler::*DNSResponseEventFunction)(DNSResponseEvent&);

#define EVT_DNS_RESPONSE(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( DNSRESPONSE_EVENT, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) \
    wxStaticCastEvent( DNSResponseEventFunction, & fn ), (wxObject *) NULL ),


DNSResolver::DNSResolver(wxEvtHandler *response_handler) : m_response_handler(response_handler),
		m_finish(false),
		m_mutex(), 
		m_work_semaphore(0, 2) {
}

void DNSResolver::Resolve(wxString address) {
	wxMutexLocker lock(m_mutex);
	m_address = address.c_str();

	m_work_semaphore.Post();
}

void DNSResolver::Finish() {
	wxMutexLocker lock(m_mutex);
	m_finish = true;
	m_work_semaphore.Post();
}

namespace {
void ares_cb(void *arg, int status, int timeouts, struct hostent *host) {
	uint32_t *r = (uint32_t*)(arg);
	if (status != ARES_SUCCESS) {
		*r = 0;
	} else {
		assert(host->h_length == 4);
		*r = *((uint32_t*)host->h_addr_list[0]);
	}

}
}

void DNSResolver::DoResolve() {

	ares_channel channel;
	int status = ares_init(&channel);
	if (status != ARES_SUCCESS) {
		DNSResponseEvent event(L"");
		wxPostEvent(m_response_handler, event);
		return;
	}

	uint32_t result = 0;
	bool end = false;

	m_mutex.Lock();
	std::wstring address = m_address.c_str();
	m_mutex.Unlock();

	ares_gethostbyname(channel, (const char*)SC::S2U(address).c_str(), AF_INET, ares_cb, &result);
	do {
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 500000;

		fd_set read_fds, write_fds
#ifdef MINGW32
		       , err_fds
#endif
				;

		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
#ifdef MINGW32
		FD_ZERO(&err_fds);
#endif
		int nfds = ares_fds(channel, &read_fds, &write_fds);
		if (nfds == 0)
			break;

		bool any_in = false;
		bool any_out = false;
		for (int i = 0; i < nfds + 1; i++) {
			if (FD_ISSET(i, &read_fds)) {
				any_in = true;
#ifdef MINGW32
				FD_SET(i, &err_fds);
#endif
			}
			if (FD_ISSET(i, &write_fds)) {
				any_out = true;
#ifdef MINGW32
				FD_SET(i, &err_fds);
#endif
			}
		}

		int i = select(nfds + 1, any_in ? &read_fds : NULL, any_out ? &write_fds : NULL, 
#ifdef MINGW32
				&err_fds, 
#else
				NULL,
#endif
				&tv);

		if (i == 0)
			break;

#ifdef MINGW32
		for (int i = 0; i < nfds + 1; i++)
			if (FD_ISSET(i, &err_fds))
				end = true;

#endif
		ares_process(channel, &read_fds, &write_fds);

	} while (!end);

	ares_destroy(channel);

	if (result) {
		uint32_t hresult= ntohl(result);

		std::wstringstream oss;

		oss << (hresult >> 24) << L".";
		oss << ((hresult >> 16) & 0xffu) << L".";
		oss << ((hresult >> 8) & 0xffu) << L".";
		oss << (hresult & 0xffu);

		DNSResponseEvent event(oss.str());
		wxPostEvent(m_response_handler, event);
	} else {
		DNSResponseEvent event(L"");
		wxPostEvent(m_response_handler, event);
	}

}

bool DNSResolver::CheckFinish() {
	wxMutexLocker lock(m_mutex);
	return m_finish;
}

void* DNSResolver::Entry() {
	while (true) {
		m_work_semaphore.Wait();

		if (CheckFinish())
			return NULL;

		DoResolve();
	}
}

RemarksConnection::RemarksConnection(wxString &address, RemarksHandler *handler, ConfigManager *config_manager) : m_resolver(this), m_config_manager(config_manager) {
	m_remarks_handler = handler;

	m_address = address;

	m_xmlrpc_connection = new XMLRPCConnection(address, this);

	m_address_resolved = false;

	m_token = -1;

	m_resolver.Create();
	m_resolver.Run();

}

bool RemarksConnection::Busy() {
	return m_command_list.size() > 0;
}

void RemarksConnection::SetIPAddress(wxString &ip) {
	m_address_resolved = false;
	m_address = ip;
}

void RemarksConnection::Login() {
	PostRequest(CreateLoginRequest());
}

void RemarksConnection::NotifyAllCommandsAboutError(const wxString& error) {
	for (std::list<boost::shared_ptr<Command> >::iterator i = m_command_list.begin();
			i != m_command_list.end();
			i++)
		(*i)->Error(error);
}

void RemarksConnection::OnDNSResponse(DNSResponseEvent &event) {
	if (!event.GetAddress().empty()) {
		m_address_resolved = true;
		m_xmlrpc_connection->SetIPAddress(event.GetAddress());

		SendCommand();
	} else {
		NotifyAllCommandsAboutError(wxString::Format(_("Failed to resolve address: %ls"), m_address.c_str()));
		m_command_list.clear();
	}
}

void RemarksConnection::FetchNewRemarks(bool notify_about_success) {
	m_command_list.push_back(boost::make_shared<FetchRemarksCommand>(this, m_remarks_handler, notify_about_success));
	TriggerRequest();
}

void RemarksConnection::FetchNewParamsAndSets(SetsParamsReceivedEvent *event) {
	m_command_list.push_back(boost::make_shared<FetchParamsAndSetsCommand>(this, m_config_manager, event));
	TriggerRequest();
}

void RemarksConnection::PostRequest(XMLRPC_REQUEST request) {
	m_xmlrpc_connection->PostRequest(request);
	XMLRPC_RequestFree(request, 1);
}

void RemarksConnection::PostRemark(Remark& remark, RemarkViewDialog* remark_view_dialog) {
	m_command_list.push_back(boost::make_shared<PostRemarkCommand>(this, remark, remark_view_dialog));
	TriggerRequest();
}

void RemarksConnection::InsertOrUpdateParam(DefinedParam *param, ParamEditControl* control, bool del) {
	m_command_list.push_back(boost::make_shared<InsertOrUpdateParamCommand>(this, param, control, del));
	TriggerRequest();
}

void RemarksConnection::InsertOrUpdateSet(DefinedDrawSet *set, SetEditControl* control, bool del) {
	m_command_list.push_back(boost::make_shared<InsertOrUpdateSetCommand>(this, set, control, del));
	TriggerRequest();
}

void RemarksConnection::TriggerRequest() {
	if (m_command_list.size() != 1)
		return;

	if (m_address_resolved == false) {
		if (!m_address.IsEmpty()) {
			m_resolver.Resolve(m_address);
		} else {
			NotifyAllCommandsAboutError(_("Connection with remarks and network sets and params server not configured"));
			m_command_list.clear();
		}
	} else if (m_token == -1) {
		Login();
	} else {
		SendCommand();	
	}

}

void RemarksConnection::SendCommand() {
	if (m_command_list.size())
		m_command_list.front()->Send();
}

XMLRPC_REQUEST RemarksConnection::CreateMethodCallRequest(const char* method_name) {
	XMLRPC_REQUEST request = XMLRPC_RequestNew();
	XMLRPC_RequestSetMethodName(request, method_name);
	XMLRPC_RequestSetRequestType(request, xmlrpc_request_call);

	XMLRPC_RequestSetData(request, XMLRPC_CreateVector(NULL, xmlrpc_vector_array));

	return request;
}

XMLRPC_REQUEST RemarksConnection::CreateMethodCallRequestWithToken(const char* method_name) {
	XMLRPC_REQUEST request = CreateMethodCallRequest(method_name);

	XMLRPC_VALUE v = XMLRPC_RequestGetData(request);
	XMLRPC_VectorAppendInt(v, NULL, m_token);

	return request;
}

XMLRPC_REQUEST RemarksConnection::CreateLoginRequest() {
	XMLRPC_REQUEST request = CreateMethodCallRequest("login");
	XMLRPC_VALUE v = XMLRPC_RequestGetData(request);

	XMLRPC_VectorAppendString(v, NULL, (const char*) SC::S2U(m_username).c_str(), 0);
	XMLRPC_VectorAppendString(v, NULL, (const char*) SC::S2U(m_password).c_str(), 0);

	return request;
}

void RemarksConnection::HandleLoginResponse(XMLRPC_REQUEST response) {
	XMLRPC_VALUE i = XMLRPC_RequestGetData(response);
	m_token = XMLRPC_GetValueInt(i);
	//fprintf(stdout, "token: %ld value_type:%d\n", m_token, XMLRPC_GetValueType(i));
}

wxString translate_error_message(wxString str) {

	if (str == _T("Server connection failure"))
		return _("Server connection failure");

	if (str == _T("Server communication error"))
		return _("Server communication error");

	if (str == _T("User is not allowed to send remarks for this base"))
		return _("User is not allowed to send remarks for this base");

	if (str == _T("Invalid response from server"))
		return _("Invalid response from server");

	if (str == _T("Incorrect username and password"))
		return _("Incorrect username and password");

	return str;
}

void RemarksConnection::HandleFault(XMLRPC_REQUEST response) {
	std::wstring fault_string;

	const char *_fault_string = XMLRPC_GetResponseFaultString(response);
	if (_fault_string)
		fault_string = SC::U2S((const unsigned char*) _fault_string);

	if (fault_string == L"Incorrect session token") {
		m_token = -1;
		Login();
		return;
	}

	wxString error = translate_error_message(fault_string);

	if (m_command_list.size()) {

		boost::shared_ptr<Command> command = m_command_list.front();
		m_command_list.pop_front();

		command->Error(error);

		SendCommand();	
	}
}

void RemarksConnection::OnXMLRPCResponse(XMLRPCResponseEvent &event) {

	XMLRPC_REQUEST response = event.GetRequest().get();

	if (XMLRPC_ResponseIsFault(response)) {
		HandleFault(response);
		return;
	}

	if (m_token == -1) {
		HandleLoginResponse(response);
		SendCommand();
	} else {
		boost::shared_ptr<Command> command = m_command_list.front();
		m_command_list.pop_front();

		size_t size = m_command_list.size();

		command->HandleResponse(response);

		if (size > 1 || (size == 1 && m_command_list.size() == 1))
			SendCommand();
	}
}

void RemarksConnection::SetUsernamePassword(wxString username, wxString password) {
	m_username = username;
	m_password = password;
}

RemarksConnection::Command::Command(RemarksConnection* connection) : m_connection(connection) {}

RemarksConnection::FetchRemarksCommand::FetchRemarksCommand(RemarksConnection* connection, RemarksHandler* remarks_handler, bool manually_triggered)
	: Command(connection), m_remarks_handler(remarks_handler), m_manually_triggered(manually_triggered)
{}

void RemarksConnection::FetchRemarksCommand::Error(wxString error) {
	if (m_manually_triggered)
		wxMessageBox(wxString::Format(_("Failed to fetch remarks: %s"), error.c_str()), _("Error"), wxICON_ERROR);
			
	else
		ErrorFrame::NotifyError(wxString::Format(_("Failed to fetch remarks: %s"), error.c_str()));
}

void RemarksConnection::FetchRemarksCommand::Send() {
	long lt = 0;
	wxConfig::Get()->Read(_T("RemarksLatestRetrieval"), &lt);

	if (lt)
		lt -= 3600;

	m_retrieval_time = time(NULL);

	XMLRPC_REQUEST request = m_connection->CreateMethodCallRequestWithToken("get_remarks");
	XMLRPC_VALUE v = XMLRPC_RequestGetData(request);
	
	XMLRPC_VectorAppendDateTime(v, NULL, lt);
	
	XMLRPC_VALUE vp = XMLRPC_CreateVector(NULL, xmlrpc_vector_array);

	std::set<std::wstring> prefixes = m_remarks_handler->GetPrefixes();
	for (std::set<std::wstring>::iterator i = prefixes.begin();
			i != prefixes.end();
			i++)
		XMLRPC_VectorAppendString(vp, NULL, (const char*) SC::S2U(*i).c_str(), 0);

	XMLRPC_AddValueToVector(v, vp);

	m_connection->PostRequest(request);
}

void RemarksConnection::FetchRemarksCommand::HandleResponse(XMLRPC_REQUEST response) {
	std::vector<Remark> rms;

	for (XMLRPC_VALUE i = XMLRPC_VectorRewind(XMLRPC_RequestGetData(response)); 
			i; 
			i = XMLRPC_VectorNext(XMLRPC_RequestGetData(response))) {

		XMLRPC_VALUE j = XMLRPC_VectorRewind(i);

		const char *doc_text = XMLRPC_GetValueBase64(j);
		xmlDocPtr remark_doc = xmlParseDoc(BAD_CAST doc_text);

		Remark r(remark_doc);

		j = XMLRPC_VectorNext(i);
		int server_id = XMLRPC_GetValueInt(j);

		j = XMLRPC_VectorNext(i);
		std::wstring prefix = SC::U2S((const unsigned char*) XMLRPC_GetValueString(j));

		j = XMLRPC_VectorNext(i);
		int id = XMLRPC_GetValueInt(j);

		r.SetId(Remark::ID(server_id, id));
		r.SetAttachedPrefix(prefix);

		rms.push_back(r);
	}

	m_remarks_handler->NewRemarks(rms, m_retrieval_time, m_manually_triggered);
}

RemarksConnection::PostRemarkCommand::PostRemarkCommand(RemarksConnection *connection, const Remark& remark, RemarkViewDialog* remark_view_dialog)
	: Command(connection), m_remark(remark), m_remark_view_dialog(remark_view_dialog) {}

void RemarksConnection::PostRemarkCommand::Error(wxString error) {
	m_remark_view_dialog->RemarkSent(false, error);
}

void RemarksConnection::PostRemarkCommand::Send() {
	XMLRPC_REQUEST request = m_connection->CreateMethodCallRequestWithToken("post_remark");
	XMLRPC_VALUE v = XMLRPC_RequestGetData(request);

	xmlChar* dumped;
	int size;
	xmlDocDumpMemory(m_remark.GetXML(), &dumped, &size);
	XMLRPC_VectorAppendBase64(v, NULL, (const char*) dumped, size);
	xmlFree(dumped);

	m_connection->PostRequest(request);
}

void RemarksConnection::PostRemarkCommand::HandleResponse(XMLRPC_REQUEST request) {
	m_remark_view_dialog->RemarkSent(true);
}

RemarksConnection::InsertOrUpdateParamCommand::InsertOrUpdateParamCommand(RemarksConnection* connection, DefinedParam* param, ParamEditControl* control, bool del) :
		Command(connection), m_control(control), m_del(del) { 
	
	m_param_xml = xmlNewDoc((const xmlChar*) "1.0");
	m_param_xml->children = param->GenerateXML(m_param_xml);

}

void RemarksConnection::InsertOrUpdateParamCommand::Error(wxString error) {
	if (m_control)
		m_control->ParamInsertUpdateError(error);
	else
		wxMessageBox(wxString::Format(_("Network operation failed: %s"), error.c_str()), _("Error"), wxICON_ERROR);
}

void RemarksConnection::InsertOrUpdateParamCommand::Send() {
	XMLRPC_REQUEST request;
	if (m_del)
		request = m_connection->CreateMethodCallRequestWithToken("remove_params");
	else
		request = m_connection->CreateMethodCallRequestWithToken("update_params");

	XMLRPC_VALUE v = XMLRPC_RequestGetData(request);

	xmlChar* dumped;
	int size;
	xmlDocDumpMemory(m_param_xml, &dumped, &size);

	XMLRPC_VALUE vp = XMLRPC_CreateVector(NULL, xmlrpc_vector_array);
	XMLRPC_VectorAppendBase64(vp, NULL, (const char*) dumped, size);
	XMLRPC_AddValueToVector(v, vp);

	xmlFree(dumped);
	m_connection->PostRequest(request);
}

void RemarksConnection::InsertOrUpdateParamCommand::HandleResponse(XMLRPC_REQUEST response) {
	XMLRPC_VALUE i = XMLRPC_VectorRewind(XMLRPC_RequestGetData(response)); 
	int v = XMLRPC_GetValueBoolean(i);	
	m_control->ParamInsertUpdateFinished(v != 0);
}

RemarksConnection::InsertOrUpdateParamCommand::~InsertOrUpdateParamCommand() {
	xmlFree(m_param_xml);
}


RemarksConnection::InsertOrUpdateSetCommand::InsertOrUpdateSetCommand(RemarksConnection* connection, DefinedDrawSet *set, SetEditControl *control, bool del) :
		Command(connection), m_control(control), m_del(del) {

	m_set_xml = xmlNewDoc((const xmlChar*) "1.0");
	m_set_xml->children = set->GenerateXML(m_set_xml);
}

void RemarksConnection::InsertOrUpdateSetCommand::Error(wxString error) {
	if (m_control)
		m_control->SetInsertUpdateError(error);
	else
		wxMessageBox(wxString::Format(_("Network operation failed: %s"), error.c_str()), _("Error"), wxICON_ERROR);
}

void RemarksConnection::InsertOrUpdateSetCommand::Send() {
	XMLRPC_REQUEST request;
	if (m_del)
		request = m_connection->CreateMethodCallRequestWithToken("remove_sets");
	else
		request = m_connection->CreateMethodCallRequestWithToken("update_sets");

	XMLRPC_VALUE v = XMLRPC_RequestGetData(request);

	xmlChar* dumped;
	int size;
	xmlDocDumpMemory(m_set_xml, &dumped, &size);

	XMLRPC_VALUE vp = XMLRPC_CreateVector(NULL, xmlrpc_vector_array);
	XMLRPC_VectorAppendBase64(vp, NULL, (const char*) dumped, size);
	XMLRPC_AddValueToVector(v, vp);

	xmlFree(dumped);

	m_connection->PostRequest(request);
}

void RemarksConnection::InsertOrUpdateSetCommand::HandleResponse(XMLRPC_REQUEST response) {
	XMLRPC_VALUE i = XMLRPC_VectorRewind(XMLRPC_RequestGetData(response)); 
	bool ok = XMLRPC_GetValueBoolean(i) != 0;
	if (m_control)
		m_control->SetInsertUpdateFinished(ok);
	else 
		if (!ok)
			wxMessageBox(_("You don't have sufficient privileges to update this set."), _("Insufficient privileges"),
				wxOK | wxICON_ERROR);
}

RemarksConnection::InsertOrUpdateSetCommand::~InsertOrUpdateSetCommand() {
	xmlFreeDoc(m_set_xml);
}


RemarksConnection::FetchParamsAndSetsCommand::FetchParamsAndSetsCommand(RemarksConnection* connection, ConfigManager* cfg_mgr, SetsParamsReceivedEvent *event) 
		: Command(connection), m_cfg_mgr(cfg_mgr), m_event(event) {}

void RemarksConnection::FetchParamsAndSetsCommand::Error(wxString error) {
	if (m_event) {
		m_event->SetsParamsReceiveError(error);
#if 0
		wxMessageBox(wxString::Format(_("Failed to fetch params and sets: %s"), error.c_str()), _("Error"), wxICON_ERROR,
			wxGetApp().GetTopWindow());
#endif
	}
}

void RemarksConnection::FetchParamsAndSetsCommand::Send() {
	long fetch_time = 0;
	wxConfig::Get()->Read(_T("NetworkParamsAndSetsLatestRetrieval"), &fetch_time);
	m_fetch_time = fetch_time;

	XMLRPC_REQUEST request = m_connection->CreateMethodCallRequestWithToken("get_params_and_sets");

	const std::set<wxString>& prefixes = m_cfg_mgr->GetDefinedDrawsSets()->GetNetworkParamAndSetsPrefixes();

	XMLRPC_VectorAppendDateTime(XMLRPC_RequestGetData(request), NULL, m_fetch_time ? m_fetch_time - 3600 : 0);

	XMLRPC_VALUE vp = XMLRPC_CreateVector(NULL, xmlrpc_vector_array);
	for (std::set<wxString>::const_iterator i = prefixes.begin();
			i != prefixes.end();
			i++)
		XMLRPC_VectorAppendString(vp, NULL, (const char*) SC::S2U(*i).c_str(), 0);

	XMLRPC_AddValueToVector(XMLRPC_RequestGetData(request), vp);

	m_connection->PostRequest(request);
}

void RemarksConnection::FetchParamsAndSetsCommand::ProcessParams(XMLRPC_VALUE vs, bool& params_updated) {
	for (XMLRPC_VALUE p = XMLRPC_VectorRewind(vs); p; p = XMLRPC_VectorNext(vs)) {

		DefinedParam *dp = new DefinedParam();
		dp->ParseXMLRPCValue(p);

		XMLRPC_VALUE v = XMLRPC_VectorNext(p);
		int d = XMLRPC_GetValueBoolean(v);

		bool updated;

		if (!d)
			updated = m_cfg_mgr->SubstituteOrAddDefinedParams(std::vector<DefinedParam*>(1, dp));
		else
			updated = m_cfg_mgr->RemoveDefinedParam(dp->GetBasePrefix(), dp->GetParamName());

		if (updated)
			params_updated = true;

	}
}

DefinedDrawInfo* RemarksConnection::FetchParamsAndSetsCommand::ProcessDrawInfo(XMLRPC_VALUE vdi) {
	DefinedDrawInfo* ddi = new DefinedDrawInfo(m_cfg_mgr->GetDefinedDrawsSets());
	ddi->ParseXMLRPCValue(vdi);
	return ddi;
}

void RemarksConnection::FetchParamsAndSetsCommand::ProcessSets(XMLRPC_VALUE ss, bool& sets_updated) {
	for (XMLRPC_VALUE s = XMLRPC_VectorRewind(ss); s; s = XMLRPC_VectorNext(ss)) {
		bool updated;

		XMLRPC_VALUE v = XMLRPC_VectorRewind(s);
		std::wstring sname = SC::U2S((const unsigned char*) XMLRPC_GetValueString(v));

		v = XMLRPC_VectorNext(s);
		XMLRPC_VALUE dv = XMLRPC_VectorRewind(v);
		if (XMLRPC_GetValueBoolean(dv)) {
			DefinedDrawSet* set = new DefinedDrawSet(m_cfg_mgr->GetDefinedDrawsSets(), true);
			set->SetName(sname);

			dv = XMLRPC_VectorNext(v);
			std::wstring uname = SC::U2S((const unsigned char*) XMLRPC_GetValueString(dv));
			set->SetUserName(uname);

			dv = XMLRPC_VectorNext(v);
			set->SetModificationTime(XMLRPC_GetValueDateTime(dv));

			std::vector<DefinedDrawInfo*> draws;
			dv = XMLRPC_VectorNext(v);
			for (XMLRPC_VALUE i = XMLRPC_VectorRewind(dv);
					i;
					i = XMLRPC_VectorNext(dv))
				set->Add(ProcessDrawInfo(i));

			updated = AddSet(set);
		} else {
			updated = m_cfg_mgr->GetDefinedDrawsSets()->RemoveSet(sname);
		}

		if (updated)
			sets_updated = true;
	}
}

bool RemarksConnection::FetchParamsAndSetsCommand::AddSet(DefinedDrawSet *set) {
	DefinedDrawsSets* def_sets = m_cfg_mgr->GetDefinedDrawsSets();

	SetsNrHash prefixes = set->GetPrefixes();
	std::set<wxString> missing_prefixes;

	for (SetsNrHash::iterator i = prefixes.begin(); i != prefixes.end(); i++)
		if (m_cfg_mgr->GetConfigByPrefix(i->first, false) == NULL)
			missing_prefixes.insert(i->first);

	if (missing_prefixes.size() != prefixes.size()) {
		for (std::set<wxString>::iterator i = missing_prefixes.begin(); i != missing_prefixes.end(); i++)
			m_cfg_mgr->GetConfigByPrefix(*i, true);

		for (SetsNrHash::iterator i = prefixes.begin(); i != prefixes.end(); i++)
			set->SyncWithPrefix(i->first);
	}

	return def_sets->AddSet(set);

}

void RemarksConnection::FetchParamsAndSetsCommand::HandleResponse(XMLRPC_REQUEST request) {
	XMLRPC_VALUE params  = XMLRPC_VectorRewind(XMLRPC_RequestGetData(request));

	bool params_updated = false;
	ProcessParams(params, params_updated);

	XMLRPC_VALUE sets = XMLRPC_VectorNext(XMLRPC_RequestGetData(request));

	bool sets_updated = false;
	ProcessSets(sets, sets_updated);

	wxConfig::Get()->Write(_T("NetworkParamsAndSetsLatestRetrieval"), long(time(NULL)));
	wxConfig::Get()->Flush();

	if (m_event)
		m_event->SetsParamsReceived(params_updated || sets_updated);

}

RemarksConnection::~RemarksConnection() {
	m_resolver.Finish();
	m_resolver.Wait();

	delete m_xmlrpc_connection;
}

BEGIN_EVENT_TABLE(RemarksConnection, wxEvtHandler) 
	EVT_XMLRPCREQUEST(wxID_ANY, RemarksConnection::OnXMLRPCResponse)
	EVT_DNS_RESPONSE(wxID_ANY, RemarksConnection::OnDNSResponse)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(REMARKSRESPONSE_EVENT)

typedef void (wxEvtHandler::*RemarksResponseEventFunction)(RemarksResponseEvent&);

#define EVT_REMARKS_RESPONSE(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( REMARKSRESPONSE_EVENT, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) \
    wxStaticCastEvent( RemarksResponseEventFunction, & fn ), (wxObject *) NULL ),


RemarksResponseEvent::RemarksResponseEvent(std::vector<Remark> remarks, RESPONSE_TYPE response_type) : wxCommandEvent(REMARKSRESPONSE_EVENT, wxID_ANY) {
	m_remarks = remarks;
	m_response_type = response_type;
}

std::vector<Remark>& RemarksResponseEvent::GetRemarks() {
	return m_remarks;
}

RemarksResponseEvent::RESPONSE_TYPE RemarksResponseEvent::GetResponseType() {
	return m_response_type;
}

wxEvent* RemarksResponseEvent::Clone() const {
	return new RemarksResponseEvent(*this);
}

RemarksStorage::Query::Query(RemarksStorage* storage) : m_storage(storage) {}

RemarksStorage::StoreRemarksQuery::StoreRemarksQuery(RemarksStorage *storage, std::vector<Remark> &remarks, time_t fetch_time, bool manually_triggered) : Query(storage), m_remarks(remarks), m_fetch_time(fetch_time), m_manually_triggered(manually_triggered) {}

void RemarksStorage::StoreRemarksQuery::Execute() {
	m_storage->ExecuteStoreRemarks(m_remarks, m_fetch_time, m_manually_triggered);
}

RemarksStorage::FetchAllRemarksQuery::FetchAllRemarksQuery(RemarksStorage *storage, std::wstring prefix, wxEvtHandler *receiver) :
	Query(storage), m_prefix(prefix), m_receiver(receiver)
{}

void RemarksStorage::FetchAllRemarksQuery::Execute() {
	std::vector<Remark> remarks = m_storage->ExecuteGetAllRemarks(m_prefix);

	RemarksResponseEvent e(remarks, RemarksResponseEvent::BASE_RESPONSE);
	wxPostEvent(m_receiver, e);
}


RemarksStorage::FetchRemarksQuery::FetchRemarksQuery(RemarksStorage *storage, 
		std::wstring prefix,
		std::wstring set, 
		time_t from_time,
		time_t to_time,
		wxEvtHandler *receiver) : Query(storage), m_prefix(prefix), m_set(set), m_from_time(from_time), m_to_time(to_time), m_receiver(receiver)
{}

void RemarksStorage::FetchRemarksQuery::Execute() {
	std::vector<Remark> remarks = m_storage->ExecuteGetRemarks(m_prefix, m_set, m_from_time, m_to_time);

	RemarksResponseEvent e(remarks, RemarksResponseEvent::RANGE_RESPONE);
	wxPostEvent(m_receiver, e);
}

RemarksStorage::FinishQuery::FinishQuery(RemarksStorage *storage) : Query(storage) {}

void RemarksStorage::FinishQuery::Execute() {
	m_storage->m_done = true;
}

RemarksStorage::RemarksStorage(RemarksHandler *remarks_handler) :
		m_sqlite(NULL),
		m_fetch_remarks_query(NULL),
		m_fetch_prefixes_query(NULL),
		m_add_prefix_query(NULL),
		m_fetch_all_remarks_query(NULL) {

	m_remarks_handler = remarks_handler;

	wxFileName szarp_dir(wxGetHomeDir(), wxEmptyString);
	szarp_dir.AppendDir(_T(".szarp"));

	wxFileName db_path(szarp_dir.GetFullPath(), _T("remarks.db"));

	m_done = false;

	bool fresh_base = false;

	if (!db_path.FileExists()) {
		if (szarp_dir.DirExists() == false)
			if (szarp_dir.Mkdir() == false)
				return;

		fresh_base = true;
	}

	int ret = sqlite3_open((const char*)SC::S2U(db_path.GetFullPath()).c_str(), &m_sqlite);
	if (ret) {
		m_sqlite = NULL;
		return;
	}

	if (fresh_base) {
		bool ok = CreateTable();
		if (ok == false) {
			sqlite3_close(m_sqlite);
			m_sqlite = NULL;
			return;
		}
	}

	PrepareQuery();

	 if (!fresh_base) {
		while ((ret = sqlite3_step(m_fetch_prefixes_query)) == SQLITE_ROW) {
			m_prefixes.insert(SC::U2S((unsigned const char*) sqlite3_column_text(m_fetch_prefixes_query, 0)));
		}
		sqlite3_reset(m_fetch_prefixes_query);
	}

}

void RemarksStorage::PrepareQuery() {
	int ret;

	ret = sqlite3_prepare_v2(
			m_sqlite, 
			"SELECT content, server_id, id FROM remark "
			"	WHERE prefix_id = (SELECT id FROM prefix WHERE name = ?1) "
			"		AND (set_name = ?2 OR set_name ISNULL) "
			"		AND time >= ?3 AND time < ?4;",
			-1,
			&m_fetch_remarks_query,
			NULL);
	assert(ret == SQLITE_OK);

	ret = sqlite3_prepare_v2(
			m_sqlite, 
			"INSERT INTO remark (server_id, id, prefix_id, set_name, time, content) "
			"	VALUES (?1, ?2, (SELECT id FROM prefix WHERE name = ?3), ?4, ?5, ?6);",
			-1,
			&m_store_remark_query,
			NULL);
	assert(ret == SQLITE_OK);
	
	ret = sqlite3_prepare_v2(
			m_sqlite, 
			"SELECT name FROM prefix;",
			-1,
			&m_fetch_prefixes_query,
			NULL);
	assert(ret == SQLITE_OK);

	ret = sqlite3_prepare_v2(
			m_sqlite, 
			"INSERT INTO prefix (name) VALUES (?1);",
			-1,
			&m_add_prefix_query,
			NULL);
	assert(ret == SQLITE_OK);

	ret = sqlite3_prepare_v2(
			m_sqlite, 
			"SELECT" 
			"	content, server_id, id "
			"FROM "
			"	remark "
			"WHERE "
			"	prefix_id = ( "
			"			SELECT "
			"				id "
			"			FROM "
			"				prefix "
			"			WHERE "
			"				name = ?1 "
			"			)",
			-1,
			&m_fetch_all_remarks_query,
			NULL);
	assert(ret == SQLITE_OK);
}

bool RemarksStorage::CreateTable() {
	const char* create_commands[]  = {
			"CREATE TABLE prefix ("
				"id INTEGER PRIMARY KEY AUTOINCREMENT,"
				"name TEXT"
				");",
			"CREATE TABLE remark ("
			"	id INTEGER,"
			"	server_id INTEGER,"
			"	prefix_id INTEGER,"
			"	set_name TEXT,"
			"	time INTEGER,"
			"	content TEXT,"
			"	PRIMARY KEY (server_id, id)"
			"	);",
			"CREATE INDEX index1 ON remark (prefix_id);",
			"CREATE INDEX index2 ON remark (set_name);",
			"CREATE INDEX index3 ON remark (time);" };
	for (size_t i = 0; i < sizeof(create_commands) / sizeof(create_commands[0]); ++i) {
		int ret = sqlite3_exec(m_sqlite, 
				create_commands[i],
				NULL,
				NULL,
				NULL);
		assert (ret == SQLITE_OK);
	}

	return true;

}

std::set<std::wstring> RemarksStorage::GetPrefixes() {
	wxMutexLocker lock(m_prefixes_mutex);
	return m_prefixes;
}

void RemarksStorage::AddPrefix(std::wstring prefix) {
	int ret;

	ret = sqlite3_bind_text(m_add_prefix_query,
			1,
			(const char*) SC::S2U(prefix).c_str(),
			-1,
			SQLITE_TRANSIENT);
	assert(ret == SQLITE_OK);

	ret = sqlite3_step(m_add_prefix_query);
	assert(ret == SQLITE_DONE);

	sqlite3_reset(m_add_prefix_query);
	sqlite3_clear_bindings(m_add_prefix_query);

}

void RemarksStorage::ExecuteStoreRemarks(std::vector<Remark>& remarks, time_t fetch_time, bool manually_triggered) {
	int ret;

	wxMutexLocker lock(m_prefixes_mutex);

	std::vector<Remark> added_remarks;

	for (std::vector<Remark>::iterator i = remarks.begin();
			i != remarks.end();
			i++) {
		std::wstring prefix = i->GetAttachedPrefix();

		if (m_prefixes.find(prefix) == m_prefixes.end()) {
			AddPrefix(prefix);
			m_prefixes.insert(prefix);
		}

		Remark::ID id = i->GetId();

		ret = sqlite3_bind_int(m_store_remark_query, 
				1,
				id.first);
		assert(ret == SQLITE_OK);

		ret = sqlite3_bind_int(m_store_remark_query, 
				2,
				id.second);
		assert(ret == SQLITE_OK);

		ret = sqlite3_bind_text(m_store_remark_query, 
				3, 
				(const char*) SC::S2U(prefix).c_str(),
				-1,
				SQLITE_TRANSIENT);
		assert(ret == SQLITE_OK);

		std::wstring set = i->GetSet();
		if (!set.empty()) {
			ret = sqlite3_bind_text(m_store_remark_query, 
					4, 
					(const char*) SC::S2U(i->GetSet()).c_str(),
					-1,
					SQLITE_TRANSIENT);
		} else {
			ret = sqlite3_bind_null(m_store_remark_query, 4);
		}
		assert(ret == SQLITE_OK);
		ret = sqlite3_bind_int(m_store_remark_query, 5, i->GetTime());
		assert(ret == SQLITE_OK);
		
		xmlChar *doc;
		int len;
		xmlDocDumpMemory(i->GetXML(), &doc, &len);

		ret = sqlite3_bind_text(m_store_remark_query, 
				6, 
				(const char*) doc,
				-1,
				xmlFree);
		assert(ret == SQLITE_OK);


		ret = sqlite3_step(m_store_remark_query);
		switch (ret) {
			case SQLITE_DONE:
				added_remarks.push_back(*i);
				break;
			case SQLITE_CONSTRAINT:
				break;
			default:
				assert(false);
		}

		sqlite3_reset(m_store_remark_query);
		sqlite3_clear_bindings(m_store_remark_query);
	}

	RemarksStoredEvent event(added_remarks, fetch_time, manually_triggered);
	wxPostEvent(m_remarks_handler, event);
}

void RemarksStorage::GetRemarks(const wxString& prefix,
			const wxString& set,
			const wxDateTime &start_date,
			const wxDateTime &end_date, wxEvtHandler *receiver) {

	wxMutexLocker lock(m_queue_mutex);

	m_queries.push_back(new FetchRemarksQuery(this,
				prefix.c_str(),
				set.c_str(),
				start_date.GetTicks(),
				end_date.GetTicks(),
				receiver));
	m_semaphore.Post();
}

void RemarksStorage::GetAllRemarks(const wxString& prefix, wxEvtHandler *handler) {
	wxMutexLocker lock(m_queue_mutex);

	m_queries.push_back(new FetchAllRemarksQuery(this,
				prefix.c_str(),
				handler));

	m_semaphore.Post();
}

void RemarksStorage::StoreRemarks(std::vector<Remark>& remarks, time_t fetch_time, bool manually_triggered) { 
	wxMutexLocker lock(m_queue_mutex);

	m_queries.push_back(new StoreRemarksQuery(this, remarks, fetch_time, manually_triggered));

	m_semaphore.Post();
}

void RemarksStorage::Finish() {
	wxMutexLocker lock(m_queue_mutex);

	m_queries.push_back(new FinishQuery(this));

	m_semaphore.Post();
}

std::vector<Remark> RemarksStorage::ExecuteGetRemarks(const std::wstring& prefix,
			const std::wstring& set,
			const time_t &start_date,
			const time_t &end_date) {

	std::vector<Remark> v;

	int ret;
	ret = sqlite3_bind_text(m_fetch_remarks_query, 
			1, 
			(const char*) SC::S2U(prefix).c_str(),
			-1,
			SQLITE_TRANSIENT);
	assert (ret == SQLITE_OK);

	ret = sqlite3_bind_text(m_fetch_remarks_query, 
			2, 
			(const char*) SC::S2U(set).c_str(),
			-1,
			SQLITE_TRANSIENT);
	assert(ret == SQLITE_OK);
	ret = sqlite3_bind_int(m_fetch_remarks_query, 
			3,
			start_date);
	assert(ret == SQLITE_OK);
	ret = sqlite3_bind_int(m_fetch_remarks_query, 
			4,
			end_date);
	assert(ret == SQLITE_OK);

	while ((ret = sqlite3_step(m_fetch_remarks_query)) == SQLITE_ROW) {
		const char* remark_doc = (const char*) sqlite3_column_text(m_fetch_remarks_query, 0);
		assert(remark_doc);

		xmlDocPtr dr = xmlParseMemory(remark_doc, strlen(remark_doc));
		assert(dr);

		Remark r(dr);

		int server_id = sqlite3_column_int(m_fetch_remarks_query, 1);
		int id = sqlite3_column_int(m_fetch_remarks_query, 2);

		r.SetId(Remark::ID(server_id, id));

		v.push_back(r);
	}

	sqlite3_reset(m_fetch_remarks_query);
	sqlite3_clear_bindings(m_fetch_remarks_query);

	return v;
}

std::vector<Remark> RemarksStorage::ExecuteGetAllRemarks(const std::wstring& prefix) {

	std::vector<Remark> v;

	int ret;
	ret = sqlite3_bind_text(m_fetch_all_remarks_query, 
			1, 
			(const char*) SC::S2U(prefix).c_str(),
			-1,
			SQLITE_TRANSIENT);
	assert (ret == SQLITE_OK);

	while ((ret = sqlite3_step(m_fetch_all_remarks_query)) == SQLITE_ROW) {
		const char* remark_doc = (const char*) sqlite3_column_text(m_fetch_all_remarks_query, 0);
		assert(remark_doc);

		xmlDocPtr dr = xmlParseMemory(remark_doc, strlen(remark_doc));
		assert(dr);

		Remark r(dr);

		int server_id = sqlite3_column_int(m_fetch_all_remarks_query, 1);
		int id = sqlite3_column_int(m_fetch_all_remarks_query, 2);

		r.SetId(Remark::ID(server_id, id));

		v.push_back(r);
	}

	sqlite3_reset(m_fetch_all_remarks_query);
	sqlite3_clear_bindings(m_fetch_all_remarks_query);

	return v;
}

void* RemarksStorage::Entry() {

	while (!m_done) {
		m_semaphore.Wait();

		wxMutexLocker lock(m_queue_mutex);

		Query *q = m_queries.front();

		q->Execute();

		delete q;
		m_queries.pop_front();
	}

	return NULL;
}


RemarksStorage::~RemarksStorage() {
	if (m_fetch_remarks_query)
		sqlite3_finalize(m_fetch_remarks_query);
	if (m_store_remark_query)
		sqlite3_finalize(m_store_remark_query);
	if (m_add_prefix_query)
		sqlite3_finalize(m_add_prefix_query);
	if (m_fetch_all_remarks_query)
		sqlite3_finalize(m_fetch_all_remarks_query);
	if (m_sqlite)
		sqlite3_close(m_sqlite);
}

Remark::Remark() {
	m_doc = boost::shared_ptr<xmlDoc>(xmlNewDoc(BAD_CAST "1.0"), xmlFreeDoc);

	m_doc->children = xmlNewDocNode(m_doc.get(), NULL, BAD_CAST "remark", NULL);
	xmlNewChild(m_doc->children, NULL, BAD_CAST "content", NULL);

	m_id = ID(-1, -1);
}

Remark::Remark(xmlDocPtr doc) {
	m_doc = boost::shared_ptr<xmlDoc>(doc, xmlFreeDoc);
	m_id = ID(-1, -1);
}

xmlDocPtr Remark::GetXML() {
	return m_doc.get();
}

void Remark::SetPrefix(std::wstring prefix) {
	xmlSetProp(m_doc->children, BAD_CAST "prefix", SC::S2U(prefix).c_str());
}

void Remark::SetAttachedPrefix(std::wstring prefix) {
	xmlSetProp(m_doc->children, BAD_CAST "attached_prefix", SC::S2U(prefix).c_str());
}

void Remark::SetBaseName(std::wstring base_name) {
	xmlSetProp(m_doc->children, BAD_CAST "base_name", SC::S2U(base_name).c_str());
}

void Remark::SetTitle(std::wstring title) {
	xmlSetProp(m_doc->children, BAD_CAST "title", SC::S2U(title).c_str());
}

void Remark::SetSet(std::wstring set) {
	if (!set.empty()) {
		xmlSetProp(m_doc->children, BAD_CAST "set", SC::S2U(set).c_str());
	} else {
		xmlAttrPtr attr = xmlHasProp(m_doc->children, BAD_CAST "set");
		if (attr)
			xmlRemoveProp(attr);
	}

}

void Remark::SetContent(std::wstring content_s) {
	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(m_doc.get());
	xmlNodePtr content = uxmlXPathGetNode(BAD_CAST "/remark/content", xpath_ctx);
	xmlNodePtr cdata = content->children;

	if (cdata) {
		xmlUnlinkNode(cdata);
		xmlFreeNode(cdata);
	}

	std::basic_string<unsigned char> cu8 = SC::S2U(content_s);

	cdata = xmlNewCDataBlock(m_doc.get(), cu8.c_str(), cu8.size());
	xmlAddChild(content, cdata);

	xmlXPathFreeContext(xpath_ctx);
}

void Remark::SetTime(time_t time) {
	std::wstringstream ss;
	ss << time;

	xmlSetProp(m_doc->children, BAD_CAST "time", SC::S2U(ss.str()).c_str());
}

void Remark::SetId(Remark::ID id) {
	m_id = id;
}

namespace {
std::wstring get_property(xmlDocPtr doc, const char* prop) {
	xmlChar* pv = xmlGetProp(doc->children, BAD_CAST prop);

	std::wstring ret;

	if (pv)
		ret = SC::U2S(pv);
	xmlFree(pv);

	return ret;
}

}

std::wstring Remark::GetPrefix() {
	return get_property(m_doc.get(), "prefix");
}

std::wstring Remark::GetAttachedPrefix() {
	std::wstring prefix = get_property(m_doc.get(), "attached_prefix");
	//for old remarks that do not have this property we use just 'prefix'
	if (prefix.empty())
		return GetPrefix();
	else
		return prefix;
}

std::wstring Remark::GetBaseName() {
	return get_property(m_doc.get(), "base_name");
}

std::wstring Remark::GetTitle() {
	return get_property(m_doc.get(), "title");
}

std::wstring Remark::GetAuthor() {
	return get_property(m_doc.get(), "author");
}

Remark::ID Remark::GetId() {
	return m_id;
}

std::wstring Remark::GetContent() {
	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(m_doc.get());

	xmlNodePtr content = uxmlXPathGetNode(BAD_CAST "/remark/content", xpath_ctx);
	assert(content);

	xmlNodePtr cdata = content->children;

	std::wstring ret;
	if (cdata) {
	    xmlChar* c = xmlNodeListGetString(m_doc.get(), cdata, 1);
	    if (c)
		    ret = SC::U2S(c);
	    xmlFree(c);
	}

	xmlXPathFreeContext(xpath_ctx);

	return ret;
}

std::wstring Remark::GetSet() {
	return get_property(m_doc.get(), "set");
}

time_t Remark::GetTime() {
	std::wstringstream wss;
	wss << get_property(m_doc.get(), "time");

	time_t t = -1;
	wss >> t;

	return t;
}

Remark::~Remark() {
}

RemarkViewDialog::RemarkViewDialog(wxWindow *parent, RemarksHandler *remarks_handler) {
	bool loaded = wxXmlResource::Get()->LoadDialog(this, parent, _T("ViewRemarkFrame"));

	m_remarks_handler = remarks_handler;
	assert(loaded);
}

void RemarkViewDialog::SetEditingMode(bool editing_mode) {
	XRCCTRL(*this, "RemarkTitleTextCtrl", wxTextCtrl)->SetEditable(editing_mode);
	XRCCTRL(*this, "RemarkContentTextCtrl", wxTextCtrl)->SetEditable(editing_mode);

	GetSizer()->Show(FindWindowById(wxID_CLOSE)->GetContainingSizer(), !editing_mode, true);

	GetSizer()->Show(FindWindowById(wxID_ADD)->GetContainingSizer(), editing_mode, true);

	GetSizer()->Show(FindWindowById(XRCID("REMARK_REFERRING_TO_LABEL"))->GetContainingSizer(), editing_mode, true);

	GetSizer()->Show(FindWindowById(XRCID("AuthorStaticCtrl"))->GetContainingSizer(), !editing_mode, true);

	GetSizer()->Layout();
}

int RemarkViewDialog::NewRemark(wxString prefix, wxString db_name, wxString set_name, wxDateTime time) {

	m_prefix = prefix;
	m_set_name = set_name;
	m_time = time;

	XRCCTRL(*this, "BaseNameStaticCtrl", wxStaticText)->SetLabel(db_name);
	XRCCTRL(*this, "SetStaticCtrl", wxStaticText)->SetLabel(set_name);
	XRCCTRL(*this, "TimeStaticCtrl", wxStaticText)->SetLabel(FormatTime(time, PERIOD_T_DAY));

	SetEditingMode(true);

	return ShowModal();

}

int RemarkViewDialog::ShowRemark(wxString db_name,
			wxString set_name,
			wxString author,
			wxDateTime time,
			wxString title,
			wxString content) {

	XRCCTRL(*this, "BaseNameStaticCtrl", wxStaticText)->SetLabel(db_name);
	
	if (!set_name.empty())
		XRCCTRL(*this, "SetStaticCtrl", wxStaticText)->SetLabel(set_name);
	else
		GetSizer()->Show(XRCCTRL(*this, "SetStaticCtrl", wxStaticText)->GetContainingSizer(), false, true);

	XRCCTRL(*this, "AuthorStaticCtrl", wxStaticText)->SetLabel(author);
	XRCCTRL(*this, "TimeStaticCtrl", wxStaticText)->SetLabel(FormatTime(time, PERIOD_T_DAY));

	XRCCTRL(*this, "RemarkTitleTextCtrl", wxTextCtrl)->SetValue(title);
	XRCCTRL(*this, "RemarkContentTextCtrl", wxTextCtrl)->SetValue(content);

	SetEditingMode(false);

	return ShowModal();
}

void RemarkViewDialog::OnCloseButton(wxCommandEvent &event) {
	EndModal(wxID_CLOSE);
}

void RemarkViewDialog::OnCancelButton(wxCommandEvent &event) {
	EndModal(wxID_CANCEL);
}

void RemarkViewDialog::OnGoToButton(wxCommandEvent &event) {
	EndModal(wxID_FIND);
}

void RemarkViewDialog::OnHelpButton(wxCommandEvent &event) {

	if (XRCCTRL(*this, "RemarkTitleTextCtrl", wxTextCtrl)->IsEditable())
		SetHelpText(_T("draw3-ext-remarks-editing"));
	else
		SetHelpText(_T("draw3-ext-remarks-browsing"));

	wxHelpProvider::Get()->ShowHelp(this);
}

wxString RemarkViewDialog::GetRemarkTitle() {
	return XRCCTRL(*this, "RemarkTitleTextCtrl", wxTextCtrl)->GetValue();
}

wxString RemarkViewDialog::GetRemarkContent() {
	return XRCCTRL(*this, "RemarkContentTextCtrl", wxTextCtrl)->GetValue();
}

void RemarkViewDialog::OnAddButton(wxCommandEvent &event) {

	m_remark_connection = m_remarks_handler->GetConnection();

	Remark r;
	r.SetPrefix(m_prefix.c_str());
	r.SetTime(m_time.GetTicks());
	if (XRCCTRL(*this, "CURRENT_WINDOW_RADIO", wxRadioButton)->GetValue())
		r.SetSet(m_set_name.c_str());

	r.SetTitle(GetRemarkTitle().c_str());
	r.SetContent(GetRemarkContent().c_str());

	m_remark_connection->PostRemark(r, this);

	Disable();

}

void RemarkViewDialog::RemarkSent(bool ok, wxString error) {
	Enable();

	if (ok) {
		wxMessageBox(_("Remark sent successfully."), _("Remark sent."), wxOK,
				wxGetApp().GetTopWindow());
		EndModal(wxID_OK);
	} else {
		wxMessageBox(wxString::Format(_("There was an error sending remark:'%s'."), error.c_str()), _("Error."),
				wxICON_ERROR, wxGetApp().GetTopWindow());
	}
}

BEGIN_EVENT_TABLE(RemarkViewDialog, wxDialog)
	LOG_EVT_BUTTON(wxID_CLOSE, RemarkViewDialog , OnCloseButton, "remarksview:close" )
	LOG_EVT_BUTTON(wxID_CANCEL, RemarkViewDialog , OnCancelButton, "remarksview:cancel" )
	LOG_EVT_BUTTON(wxID_HELP, RemarkViewDialog , OnHelpButton, "remarksview:help" )
	LOG_EVT_BUTTON(XRCID("GOTO_BUTTON"), RemarkViewDialog , OnGoToButton, "remarksview:goto" )
	LOG_EVT_BUTTON(wxID_ADD, RemarkViewDialog , OnAddButton, "remarksview:add" )
END_EVENT_TABLE()

