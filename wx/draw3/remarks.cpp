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
#include <sstream>

#include <wx/config.h>
#include <wx/statline.h>
#include <wx/file.h>
#include <wx/dir.h>
#include <wx/xrc/xmlres.h>
#include <wx/config.h>

#include "cconv.h"

#include "remarks.h"
#include "cfgmgr.h"
#include "biowxsock.h"
#include "authdiag.h"

#include <wx/listimpl.cpp>

#ifndef NO_SQLITE3
#ifndef NO_VMIME

WX_DEFINE_LIST(NNTPTaskQueue)

NNTPStatus::NNTPStatus() {
	m_x = m_y = m_z = 0;
}

NNTPStatus::TYPE NNTPStatus::GetType() const {
	return (TYPE)m_x;
}

int NNTPStatus::GetValue() const {
	return m_x * 100 + m_y * 10 + m_z;
}

namespace {

struct status_code {
	const int value;
	const wxChar* const text;
};

const status_code status_codes[] = {
	{ 0, _T("unrecognized response from server") },
	{ 100, _T("help text") },
	{ 199, _T("debug output") },

	{ 200, _T("server ready - posting allowed") },
	{ 201, _T("server ready - no posting allowed") },
	{ 202, _T("slave status noted") },
	{ 205, _T("closing connection - goodbye!") },
	{ 211, _T("group selected") },
	{ 215, _T("list of newsgroups follows") },
	{ 220, _T("article retrieved - head and body follow") },
	{ 221, _T("article retrieved - head follows") },
	{ 222, _T("article retrieved - body follows") },
	{ 223, _T("article retrieved - request text separately") },
	{ 230, _T("list of new articles by message-id follows") },
	{ 231, _T("list of new newsgroups follows") },
	{ 235, _T("article transferred ok") },
	{ 240, _T("article posted ok")  },

	{ 335, _T("send article to be transferred.") },
	{ 340, _T("send article to be posted.") },

	{ 400, _T("service discontinued") },
	{ 411, _T("no such news group") },
	{ 412, _T("no newsgroup has been selected") },
	{ 420, _T("no current article has been selected") },
	{ 421, _T("no next article in this group") },
	{ 422, _T("no previous article in this group") },
	{ 423, _T("no such article number in this group") },
	{ 430, _T("no such article found") },
	{ 435, _T("article not wanted - do not send it") },
	{ 436, _T("transfer failed - try again later") },
	{ 437, _T("article rejected - do not try again.") },
	{ 440, _T("posting not allowed") },
	{ 441, _T("posting failed") },

	{ 500, _T("command not recognized") },
	{ 501, _T("command syntax error") },
	{ 502, _T("access restriction or permission denied") },
	{ 503, _T("program fault - command not performed") },
	
};

};

const wxChar* NNTPStatus::GetDescription() const {
	int value = GetValue();
	for (size_t i = 0; i < sizeof(status_codes) / sizeof(status_code); ++i) {
		if (status_codes[i].value == value)
			return status_codes[i].text;
	}

	return _T("Unknown status code");
}

bool NNTPStatus::Parse(const std::string& str) {
	if (str.size() < 3)
		return false;

	int x, y, z;
	x = str[0] - '0';
	y = str[1] - '0';
	z = str[2] - '0';

	if (9 >= x && x >= 0 
			&& 9 >= y && y >= 0 
			&& 9 >= z && z >= 0)
		return false;

	m_x = x;
	m_y = y;
	m_z = z;

	return true;
}

const int NNTPSSLConnection::input_buffer_size = 1024;

NNTPSSLConnection::NNTPSSLConnection(wxEvtHandler *handler) :
	m_socket(NULL),	
	m_ssl(NULL),
	m_ssl_ctx(NULL),
	m_output_buffer(NULL),
	m_output_buffer_size(0),
	m_output_buffer_len(0),
	m_input_buffer((char*)malloc(input_buffer_size)),
	m_input_buffer_size(input_buffer_size),
	m_input_buffer_len(0),
	m_handler(NULL),
	m_state(NOT_CONNECTED)
{
}

bool NNTPSSLConnection::IsConnected() const {
	return m_state != NOT_CONNECTED;
}

bool NNTPSSLConnection::Connect(wxIPV4address address) {
	if (m_state != NOT_CONNECTED)
		return false;

	assert(m_socket == NULL);

	m_socket = new wxSocketClient(wxSOCKET_NOWAIT);
	m_socket->SetNotify(wxSOCKET_INPUT_FLAG |
			wxSOCKET_OUTPUT_FLAG |
			wxSOCKET_CONNECTION_FLAG |
			wxSOCKET_LOST_FLAG);
	m_socket->SetEventHandler(*this);
	m_socket->Notify(true);

	m_state = CONNECTING;

	bool ret = m_socket->Connect(address);
	if (ret == true) {
		ConnectSSL();
		return true;
	}

	if (m_socket->Error()
		&& m_socket->LastError() != wxSOCKET_WOULDBLOCK) 
		return true;

	Cleanup();

	return false;
}

void NNTPSSLConnection::ConnectionTerminated() {
	Cleanup();
	NNTPConnectionEvent event(NNTPConnectionEvent::CONNECTION_LOST);	
	m_handler->ProcessEvent(event);
}

void NNTPSSLConnection::SocketConnected() {
	m_state = IDLE;
	NNTPConnectionEvent event(NNTPConnectionEvent::CONNECTION_READY);
	m_handler->ProcessEvent(event);
}

void NNTPSSLConnection::ConnectSSL() {

	if (m_state != CONNECTING_SSL) {
		assert(m_ssl == NULL);
		assert(m_ssl_ctx == NULL);	

		m_ssl_ctx = SSL_CTX_new(SSLv23_client_method());

		m_ssl = SSL_new(m_ssl_ctx);	
		BIO *bio = BIOSocketClientNew(m_socket);
		SSL_set_bio(m_ssl, bio, bio);

		m_state = CONNECTING_SSL;
	}

	while (true) {
		int ret = SSL_connect(m_ssl);
		if (ret == 1) {
			SocketConnected();
			break;
		}

		int error = SSL_get_error(m_ssl, ret);

		switch (error) {
			case SSL_ERROR_SYSCALL:
				if (ret == 0) {
					ConnectionTerminated();
					return;
				} else if (errno == EINTR)
					continue;
				else {
					Error(strerror(errno));
					return;
				}
			default:
				Error(ERR_error_string(error, NULL));
				return;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				return;
		}
	}
}

void NNTPSSLConnection::SocketEvent(wxSocketEvent &event) {

	wxSocketNotify type = event.GetSocketEvent();

	switch (type) {
		case wxSOCKET_LOST:
			ConnectionTerminated();
			break;
		case wxSOCKET_CONNECTION:
			assert(m_state == CONNECTING);
			SocketConnected();
			break;
		case wxSOCKET_INPUT:
			switch (m_state) {
				case CONNECTING_SSL:
					ConnectSSL();
					break;
				case WRITE_BLOCKED_ON_READ:
					WriteData();
					break;
				case IDLE:
					ReadData();
					break;
				default:
					break;

			}
			break;
		case wxSOCKET_OUTPUT:
			switch (m_state) {
				case CONNECTING_SSL:
					ConnectSSL();
					break;
				case READ_BLOCKED_ON_WRITE:
					ReadData();
					break;
				case IDLE:
					WriteData();
					break;
				default:
					break;
			}
			break;
	}
}

void NNTPSSLConnection::WriteData() {

	while (m_output_buffer_len) {
		int ret = SSL_write(m_ssl, 
				m_output_buffer, 
				m_output_buffer_len);

		if (ret <= 0) {
			int error = SSL_get_error(m_ssl, ret);
			switch (error) {
				case SSL_ERROR_SYSCALL:
					if (ret == 0)
						ConnectionTerminated();
					else if (errno == EINTR)
						continue;
					else
						Error(strerror(errno));
					break;
				default:
					Error(ERR_error_string(error, NULL));
					break;
				case SSL_ERROR_WANT_READ:
					m_state = WRITE_BLOCKED_ON_READ;
					return;
				case SSL_ERROR_WANT_WRITE:
					return;
			}
		}

		m_output_buffer_len -= ret;
		memmove(m_output_buffer, m_output_buffer + ret, m_output_buffer_len);

		if (m_output_buffer_len == 0) {
			NNTPConnectionEvent event(NNTPConnectionEvent::DATA_WRITTEN);
			m_handler->ProcessEvent(event);
		}

	}

}

bool NNTPSSLConnection::Query(const std::string& query) {
	switch (m_state) {
		case NOT_CONNECTED:
		case CONNECTING_SSL:
		case CONNECTING:
			return false;
		default:
			break;
	}

	int len = m_output_buffer_len;

	m_output_buffer_len += query.size();
	if (m_input_buffer_len > m_input_buffer_size) {
		m_output_buffer_size = m_input_buffer_len;
		m_output_buffer = (char*)realloc(m_output_buffer, m_input_buffer_size);
	}

	memcpy(m_output_buffer + len, query.c_str(), query.size());

	if (m_state == IDLE)
		WriteData();

	return true;
}

void NNTPSSLConnection::PostReadData() {
	if (m_input_buffer_len == 0)
		return;

	char *line = m_input_buffer;
	char *eol;
	while ((eol = (char*) memchr(line, '\n', m_input_buffer_len - (line - m_input_buffer)))) {
		int eol_position = eol - m_input_buffer;
		if (eol_position < 2 
				|| m_input_buffer[eol_position - 1] != '\r') {
			int followup = eol_position + 1;
			if (followup >= m_input_buffer_len)
				break;
			line = m_input_buffer + followup;
			continue;
		}

		int line_length = eol_position - 1;

		NNTPConnectionEvent event(NNTPConnectionEvent::DATA_READ, 
				std::string(m_input_buffer, line_length));
		m_handler->ProcessEvent(event);

		char *next_line = eol + 1;
		m_input_buffer_len -= next_line - m_input_buffer;
		memmove(m_input_buffer, next_line, m_input_buffer_len);
		line = m_input_buffer;
	}

}

void NNTPSSLConnection::Error(const std::string& cause) {
	Cleanup();
	NNTPConnectionEvent ev(NNTPConnectionEvent::CONNECTION_FAILED, cause);
	wxPostEvent(m_handler, ev);
}

void NNTPSSLConnection::ReadData() {

	while (true) {
		int ret = SSL_read(m_ssl, 
				m_input_buffer + m_input_buffer_len, 
				m_input_buffer_size - m_output_buffer_len - 1);

		if (ret <= 0) {
			PostReadData();
			int error = SSL_get_error(m_ssl, ret);
			switch (error) {
				case SSL_ERROR_SYSCALL:
					if (ret == 0)
						ConnectionTerminated();
					else if (errno == EINTR)
						continue;
					else
						Error(strerror(errno));
					return;
				case SSL_ERROR_WANT_WRITE:
					m_state = READ_BLOCKED_ON_WRITE;
					return;
				case SSL_ERROR_WANT_READ:
					return;
				default:
					Error(ERR_error_string(error, NULL));
					break;
			}
		}

		m_output_buffer_len += ret;

		if (m_output_buffer_len == m_output_buffer_size - 1) 
			PostReadData();

	}

}


void NNTPSSLConnection::Cleanup() {
	if (m_socket) {
		m_socket->Destroy();
		m_socket = NULL;
	}

	if (m_ssl) {
		SSL_free(m_ssl);
		m_ssl = NULL;
	}

	if (m_ssl_ctx) {
		SSL_CTX_free(m_ssl_ctx);
		m_ssl_ctx = NULL;
	}

	free(m_output_buffer);
	m_output_buffer = NULL;

	m_input_buffer_len = 0;
	m_output_buffer_size = 0;
	m_output_buffer_len = 0;
}

NNTPSSLConnection::~NNTPSSLConnection() {
	Cleanup();
	free(m_input_buffer);
}

BEGIN_EVENT_TABLE(NNTPSSLConnection, wxEvtHandler)
    EVT_SOCKET(wxID_ANY, NNTPSSLConnection::SocketEvent)
END_EVENT_TABLE()


DECLARE_EVENT_TYPE(NNTP_CONN_EVENT, -1)
DEFINE_EVENT_TYPE(NNTP_CONN_EVENT)

typedef void (wxEvtHandler::*NNTPConnectionEventFunction)(NNTPConnectionEvent&);

#define EVT_NNTP_CONN(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( NNTP_CONN_EVENT, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) \
    wxStaticCastEvent( NNTPConnectionEventFunction, & fn ), (wxObject *) NULL ),

NNTPConnectionEvent::NNTPConnectionEvent(Type type, const std::string& data) : m_type(type), m_data(data) {}

wxEvent* NNTPConnectionEvent::Clone() const {
	return new NNTPConnectionEvent(*this);
}

NNTPConnectionEvent::Type NNTPConnectionEvent::GetEventType() const {
	return m_type;
}

const std::string& NNTPConnectionEvent::GetData() const {
	return m_data;
}


void GetGroupsTask::SetGroups(const std::vector<std::string>& groups) {
	m_groups = groups;
}

PostMessageTask::PostMessageTask(vmime::ref<vmime::message> msg, const std::string& group)  : m_msg(msg), m_group(group)
{}

vm::ref<vm::message> PostMessageTask::GetMessage() const {
	return m_msg;
}

const std::string& PostMessageTask::GetGroup() const {
	return m_group;
}

PostMessageTask::~PostMessageTask() {
}

FetchMessageTask::FetchMessageTask(const std::string& msg_id) 
	: m_msg_id(msg_id) {}

const std::string& FetchMessageTask::GetMsgId() const {
	return m_msg_id;
}

void FetchMessageTask::SetArticle(vm::ref<vm::message> article) {
	m_article = article;
}

vm::ref<vm::message> FetchMessageTask::GetArticle() {
	return m_article;
}

FetchNewMessagesTask::FetchNewMessagesTask(time_t start_time, const std::string& group) : 
	m_start_time(start_time),
	m_group(group)
{}

void FetchNewMessagesTask::SetIds(const std::vector<std::string> &ids) {
	m_ids = ids;
}

const std::string& FetchNewMessagesTask::GetGroup() const {
	return m_group;
}

time_t FetchNewMessagesTask::GetStartTime() const {
	return m_start_time;
}

NNTPTaskExecutor::NNTPTaskExecutor(NNTPConnectionData &data, NNTPClient *client) : 
		m_connection_data(data),
		m_client(client),
		m_connection(this), 
		m_queue() {

	m_states[_T("NNTPConnecting")] 
		= new NNTPConnecting(m_connection_data, m_queue, m_connection, *this);
	m_states[_T("NNTPAuthenticating")] 
		= new NNTPAuthenticating(m_connection_data, m_queue, m_connection, *this);
	m_states[_T("NNTPReady")] 
		= new NNTPReady(m_connection_data, m_queue, m_connection, *this);
	m_states[_T("NNTPPostingArticle")] 
		= new NNTPPostingArticle(m_connection_data, m_queue, m_connection, *this);
	m_states[_T("NNTPFetchingArticle")] 
		= new NNTPFetchingArticle(m_connection_data, m_queue, m_connection, *this);
	m_states[_T("NNTPFetchingGroups")] 
		= new NNTPFetchingArticle(m_connection_data, m_queue, m_connection, *this);
	m_states[_T("NNTPFetchingNewMessages")] 
		= new NNTPFetchingNewMessages(m_connection_data, m_queue, m_connection, *this);

	EnterState(_T("NNTPReady"));
}

bool NNTPTaskExecutor::CheckStatusCode(const NNTPConnectionEvent &event, NNTPStatus::TYPE type) {
	bool ret = false;

	NNTPStatus sc;
	if (!sc.Parse(event.GetData()) || sc.GetType() != type) {
		HandleNNTPError(sc);
		ret = false;
	} else
		ret = true;

	return ret;
}

void NNTPTaskExecutor::TaskCompleted(NNTPTaskCompleted *completed) {
	m_client->TaskCompleted(completed);
}

void NNTPTaskExecutor::HandleNNTPError(NNTPStatus &sc) {
	NNTPTaskCompleted* tc = new NNTPTaskCompleted();

	tc->m_task = m_queue.GetFirst()->GetData();
	tc->m_is_error = true;
	tc->m_error.SetNNTPError(sc);
	m_queue.Erase(0);

	TaskCompleted(tc);
	EnterState(_T("NNTPReady"));
}

void NNTPTaskExecutor::OnConnectionEvent(NNTPConnectionEvent &event) {
	m_state->HandleConnectionEvent(event);
}

void NNTPTaskExecutor::EnterState(const wxString& state) {
	StatesHash::iterator i = m_states.find(state);

	if (i == m_states.end()) {
		wxLogError(_T("State %s not found"), state.c_str());
		assert(false);
	}
	
	m_state = i->second;
	m_state->Enter();
}

void NNTPTaskExecutor::ExecuteTask(NNTPTask *task) {
	m_queue.Append(task);
}

NNTPTaskExecutor::~NNTPTaskExecutor() {
	for (StatesHash::iterator i = m_states.begin();
			i != m_states.end();
			i++)
		delete i->second;
}

BEGIN_EVENT_TABLE(NNTPTaskExecutor, wxEvtHandler)
	EVT_NNTP_CONN(wxID_ANY, NNTPTaskExecutor::OnConnectionEvent)
END_EVENT_TABLE()

NNTPTaskExecutorState::NNTPTaskExecutorState(NNTPConnectionData &data,
			NNTPTaskQueue &queue,
			NNTPSSLConnection &connection,
			NNTPTaskExecutor& client) : 
		m_connection_data(data),
		m_queue(queue),
		m_connection(connection),
		m_executor(client) 
{}

void NNTPTaskExecutorState::TaskReceived() {
}

void NNTPTaskExecutorState::Enter() {
	if (m_connection.IsConnected() == false) {
		m_executor.EnterState((_T("NNTPConnecting")));
		return;
	}
	StateEntered();
}

void NNTPTaskExecutorState::HandleConnectionEvent(const NNTPConnectionEvent &event) {
	switch (event.GetEventType()) {
		case NNTPConnectionEvent::DATA_READ:
			break;
		case NNTPConnectionEvent::CONNECTION_FAILED:
		case NNTPConnectionEvent::CONNECTION_LOST:
			m_executor.EnterState(_T("NNTPConnecting"));
		default:
			return;
			break;
	}

	const std::string& line = event.GetData();
	if (m_awaiting_status) {
		NNTPStatus status;
		status.Parse(line);
		StatusRead(status, line);
	} else 
		LineRead(line);

}

NNTPTaskExecutorState::~NNTPTaskExecutorState() {}
	
NNTPConnecting::NNTPConnecting(NNTPConnectionData &connection_data, 
			NNTPTaskQueue &queue, 
			NNTPSSLConnection &connection,
			NNTPTaskExecutor& client)
	: NNTPTaskExecutorState(connection_data, queue, connection, client),
	m_failed_connects(0)
{}

void NNTPConnecting::StateEntered() {
}

void NNTPConnecting::StatusRead(NNTPStatus &status, const std::string &line) {
}

void NNTPConnecting::Enter() {
	m_failed_connects = 0;
	m_connection.Connect(m_connection_data.address);
}

void NNTPConnecting::LineRead(const std::string &line) 
{}

void NNTPConnecting::HandleConnectionEvent(const NNTPConnectionEvent &event) {
	switch (event.GetEventType()) {
		case NNTPConnectionEvent::CONNECTION_FAILED:
		case NNTPConnectionEvent::CONNECTION_LOST:
			m_failed_connects++;
			if (m_failed_connects < 4) 
				m_connection.Connect(m_connection_data.address);
			else {

				NNTPTaskCompleted* tc = new NNTPTaskCompleted();

				tc->m_task = m_queue.GetFirst()->GetData();
				tc->m_is_error = true;
				tc->m_error.SetNetworkError(_("Failed to connect to remarks server"));
				m_queue.Erase(0);

				m_executor.TaskCompleted(tc);
				m_executor.EnterState(_T("NNTPReady"));
			}
			break;
		case NNTPConnectionEvent::CONNECTION_READY:
			m_failed_connects = 0;
			break;
		case NNTPConnectionEvent::DATA_READ: {
			NNTPStatus sc;
			if (m_executor.CheckStatusCode(event, NNTPStatus::COMMAND_OK)) 
				m_executor.EnterState(_T("NNTPAuthenticating"));
			break;
			}
		default:
			break;
	}
}


size_t NNTPTask::free_task_id = 0;

NNTPTask::NNTPTask() {
	m_task_id = free_task_id++;
}

size_t NNTPTask::GetTaskId() {
	return m_task_id;
}
NNTPAuthenticating::NNTPAuthenticating(NNTPConnectionData &connection_data, 
			NNTPTaskQueue &queue, 
			NNTPSSLConnection &connection,
			NNTPTaskExecutor& client)
	: NNTPTaskExecutorState(connection_data, queue, connection, client)
{
}

void NNTPAuthenticating::StateEntered() {
	std::string line = std::string("authinfo user ") 
		+ m_connection_data.username 
		+ "\r\n";
	m_awaiting_status = true;
	m_connection.Query(line);

}

void NNTPAuthenticating::StatusRead(NNTPStatus &status, const std::string &line) {
	std::string query;
	NNTPStatus sc;
	sc.Parse(line);
	switch (sc.GetType()) {
		case NNTPStatus::COMMAND_OK_CONTINUE:
			query = std::string("authinfo pass ") 
				+ m_connection_data.password
				+ "\r\n";
			m_connection.Query(query);
			break;
		case NNTPStatus::COMMAND_OK:
			m_executor.EnterState(_T("NNTPReady"));
			break;
		default:
			m_executor.HandleNNTPError(sc);
	}
}

void NNTPAuthenticating::LineRead(const std::string& line) {
}

NNTPReady::NNTPReady(NNTPConnectionData &connection_data, 
			NNTPTaskQueue &queue, 
			NNTPSSLConnection &connection,
			NNTPTaskExecutor& client)
	: NNTPTaskExecutorState(connection_data, queue, connection, client)
{}

void NNTPReady::DispatchTask() {
	if (m_queue.GetCount() == 0)
		return;

	NNTPTask* task = m_queue.GetFirst()->GetData();

	if (dynamic_cast<PostMessageTask*>(task))
		m_executor.EnterState(_T("NNTPPostingArticle"));
	else if (dynamic_cast<FetchMessageTask*>(task))
		m_executor.EnterState(_T("NNTPFetchingArticle"));
	else if (dynamic_cast<FetchNewMessagesTask*>(task))
		m_executor.EnterState(_T("NNTPFetchingNewMessages"));
	else if (dynamic_cast<GetGroupsTask*>(task))
		m_executor.EnterState(_T("NNTPFetchingGroups"));
	else 
		assert(false);
	return;
}

void NNTPReady::StateEntered() {
	DispatchTask();
}

void NNTPReady::LineRead(const std::string &line) {
}

void NNTPReady::StatusRead(NNTPStatus &status, const std::string &line) {
}

void NNTPReady::TaskReceived() {
	DispatchTask();
}

NNTPFetchingArticle::NNTPFetchingArticle(NNTPConnectionData &connection_data, 
			NNTPTaskQueue &queue, 
			NNTPSSLConnection& connection,
			NNTPTaskExecutor& client)
	: NNTPTaskExecutorState(connection_data, queue, connection, client)
{}

void NNTPFetchingArticle::StateEntered() {
	FetchMessageTask* task = dynamic_cast<FetchMessageTask*>(m_queue.GetFirst()->GetData());

	m_awaiting_status = true;
	std::string command = "ARTICLE " + task->GetMsgId() + "\r\n";
	m_connection.Query(command);
}

void NNTPFetchingArticle::StatusRead(NNTPStatus &status, const std::string &line) {
	m_awaiting_status = false;
}

void NNTPFetchingArticle::LineRead(const std::string &line) {

	if (line != ".\r\n") {
		m_article += line;
		return;
	} else {
		vm::ref<vm::message> msg = vm::create<vm::message>();
		msg->parse(m_article);

		FetchMessageTask* tsk = dynamic_cast<FetchMessageTask*>(m_queue.GetFirst()->GetData());
		tsk->SetArticle(msg);
		
		NNTPTaskCompleted *tc = new NNTPTaskCompleted();
		tc->m_task = tsk;
		tc->m_is_error = false;

		m_queue.Erase(0);
		m_article.clear();
		m_executor.EnterState(_T("NNTPReady"));
	}
}

NNTPPostingArticle::NNTPPostingArticle(NNTPConnectionData &connection_data, 
			NNTPTaskQueue &queue, 
			NNTPSSLConnection &connection,
			NNTPTaskExecutor& client)
	: NNTPTaskExecutorState(connection_data, queue, connection, client)
{}

void NNTPPostingArticle::StateEntered() {

	PostMessageTask* task = dynamic_cast<PostMessageTask*>(m_queue.GetFirst()->GetData());

	m_substate = CHANGING_GROUP;
	m_awaiting_status = true;

	std::string query = "GROUP " + task->GetGroup() + "\r\n";
	m_connection.Query(query);
}

void NNTPPostingArticle::StatusRead(NNTPStatus &status, const std::string &line) {
	PostMessageTask* task = dynamic_cast<PostMessageTask*>(m_queue.GetFirst()->GetData());

	switch (m_substate) {
		case CHANGING_GROUP:
			if (status.GetType() == NNTPStatus::COMMAND_OK) {
				m_substate = REQUESTING_POST;
				m_connection.Query("POST");
			} else
				m_executor.HandleNNTPError(status);

			m_substate = REQUESTING_POST;
			break;
		case REQUESTING_POST:
			if (status.GetType() == NNTPStatus::COMMAND_OK_CONTINUE) {
				m_substate = POSTING;
				m_connection.Query(task->GetMessage()->generate());
				m_connection.Query(".\r\n");
			} else
				m_executor.HandleNNTPError(status);

			m_substate = POSTING;
			break;
		case POSTING:
			if (status.GetType() == NNTPStatus::COMMAND_OK) {
				delete m_queue.GetFirst()->GetData();
				m_queue.Erase(0);
				m_executor.EnterState(_T("NNTPReady"));
			} else 
				m_executor.HandleNNTPError(status);
			break;
	}

}

void NNTPPostingArticle::LineRead(const std::string &line) {
}

NNTPFetchingGroups::NNTPFetchingGroups(NNTPConnectionData &connection_data, 
			NNTPTaskQueue &queue, 
			NNTPSSLConnection &connection,
			NNTPTaskExecutor& client)
	: NNTPTaskExecutorState(connection_data, queue, connection, client)
{}

void NNTPFetchingGroups::StateEntered() {
	m_awaiting_status = true;
	m_groups.clear();
	std::string s = "LIST\r\n";
	m_connection.Query(s);

}

void NNTPFetchingGroups::StatusRead(NNTPStatus &status, const std::string &line) {
	if (status.GetType() != NNTPStatus::COMMAND_OK)
		m_executor.HandleNNTPError(status);

	m_awaiting_status = false;
}

void NNTPFetchingGroups::LineRead(const std::string& line) {
	if (line == ".\r\n") {
		GetGroupsTask* tsk = dynamic_cast<GetGroupsTask*>(m_queue.GetFirst()->GetData());
		tsk->SetGroups(m_groups);

		NNTPTaskCompleted *tc = new NNTPTaskCompleted();
		tc->m_task = tsk;
		tc->m_is_error = false;

		m_executor.TaskCompleted(tc);

		m_queue.Erase(0);

		m_executor.EnterState(_T("NNTPReady"));
	} else {
		std::string group;
		std::istringstream is(line);
		is >> group;
		m_groups.push_back(group);
	}
}

NNTPFetchingNewMessages::NNTPFetchingNewMessages(NNTPConnectionData &connection_data, 
			NNTPTaskQueue &queue, 
			NNTPSSLConnection &connection,
			NNTPTaskExecutor& client)
	: NNTPTaskExecutorState(connection_data, queue, connection, client)
{}

void NNTPFetchingNewMessages::StateEntered() {
	FetchNewMessagesTask* task = dynamic_cast<FetchNewMessagesTask*>(m_queue.GetFirst()->GetData());

	struct tm _tm;
	time_t st = task->GetStartTime();
	gmtime_r(&st, &_tm);

	std::ostringstream os;
	os << "NEWNEWS " 
		<< task->GetGroup() << " " 
		<< (_tm.tm_year + 1900) << (_tm.tm_mon + 1) << _tm.tm_mday << " "
		<< _tm.tm_hour << _tm.tm_min << _tm.tm_sec << " GMT";

	m_connection.Query(os.str());

	m_awaiting_status = true;

}

void NNTPFetchingNewMessages::StatusRead(NNTPStatus &status, const std::string &line) {
	if (status.GetType() != NNTPStatus::COMMAND_OK) 
		m_executor.HandleNNTPError(status);
	m_awaiting_status = false;
}

void NNTPFetchingNewMessages::LineRead(const std::string &line) {
	if (line != ".\r\n") {
		std::string id = line.substr(0, line.length() - 2);	
		m_ids.push_back(id);
	} else {
		FetchNewMessagesTask *tsk = dynamic_cast<FetchNewMessagesTask*>(m_queue.GetFirst()->GetData());
		tsk->SetIds(m_ids);
		assert(tsk);

		NNTPTaskCompleted* tc = new NNTPTaskCompleted();
		tc->m_is_error = false;
		tc->m_task = tsk;

		m_executor.TaskCompleted(tc);

		m_queue.Erase(0);
		m_executor.EnterState(_T("NNTPReady"));
		return;
	}
}

void NNTPTaskError::SetNNTPError(const NNTPStatus& nntp_error) {
	m_is_nntp_error = true;
	m_nntp_error = nntp_error;
}

void NNTPTaskError::SetNetworkError(wxString error) {
	m_is_nntp_error = false;
	m_network_error = error;
}

RemarksStorage* RemarksStorage::_instance = NULL;

RemarksStorage::RemarksStorage() {
	m_sqlite = NULL;
	m_fetch_remark_info = NULL;
	m_has_remark = NULL;
}

RemarksStorage::~RemarksStorage() {
	if (m_fetch_remark_info)
		sqlite3_finalize(m_fetch_remark_info);
	if (m_has_remark)
		sqlite3_finalize(m_has_remark);
	if (m_sqlite)
		sqlite3_close(m_sqlite);
}

void RemarksStorage::Destroy() {
	delete _instance;
}

bool RemarksStorage::GetStorage(RemarksStorage **storage, wxString &error) {
	if (_instance) {
		*storage = _instance;
		return 0;
	}

	_instance = new RemarksStorage();
	if (_instance->Init(error)) {
		delete _instance;
		_instance = NULL;
		return 1;
	}
		
	*storage = _instance;
	return 0;
} 
namespace {

bool create_dir(const wxString& path, wxString &error) {

	if (!wxDir::Exists(path)) {
		int ret = mkdir(SC::S2A(path).c_str()
#ifdef __WXGTK__
				, 0644
#endif
				);

		if (ret) {
			error = _("Failed to create dir: ") 
				+ path 
				+ _T(" (")
				+ SC::A2S(strerror(errno)) + _T(")");
			return false;
		}

	}

	return true;
}

}

bool RemarksStorage::CreateTable() {

	const char* create_commands[]  = {
			"CREATE TABLE remark ("
			"	prefix TEXT,"
			"	window TEXT,"
			"	graph TEXT,"
			"	ref_time INTEGER,"
			"	title TEXT,"
			"	from TEXT,"
			"	send_time TEXT,"
			"	title TEXT,"
			"	id TEXT"
			"	);",
			"CREATE TABLE group ("
			"	name TEXT,"
			"	can_read INTEGER,"
			"	can_post INTEGER,"
			"	last_fetch_time INTEGER,"
			"	);",
			"CREATE INDEX index1 ON remark (base);",
			"CREATE INDEX index2 ON remark (window);",
			"CREATE INDEX index3 ON remark (graph);",
			"CREATE INDEX index4 ON remark (time);",
		};
	for (size_t i = 0; i < sizeof(create_commands) / sizeof(create_commands[0]); ++i) {
		int ret = sqlite3_exec(m_sqlite, 
				create_commands[i],
				NULL,
				NULL,
				NULL);
		if (ret != SQLITE_OK) 
			return false;
	}

	return true;
}

void RemarksStorage::PrepareQuery() {
	int ret;

	ret = sqlite3_prepare_v2(
			m_sqlite, 
			"SELECT id, title, author, send_time FROM remark"
			"	WHERE prefix = ?1 "
			"		AND window = ?2 "
			"		AND graph = ?3 "
			"		AND time BETWEEN ?4 AND ?5;",
			-1,
			&m_fetch_remark_info,
			NULL);
	assert(ret == SQLITE_OK);

	ret = sqlite3_prepare_v2(
			m_sqlite, 
			"SELECT COUNT(*) FROM remark "
			"	WHERE prefix = ?1 "
			"		AND window = ?2 "
			"		AND graph = ?3 "
			"		AND time BETWEEN ?4 AND ?5;",
			-1,
			&m_has_remark,
			NULL);
	assert(ret == SQLITE_OK);
}

std::vector<GroupInfo> RemarksStorage::GetGroups() {
	std::vector<GroupInfo> result;

	sqlite3_stmt *stmt;
	int ret = sqlite3_prepare_v2(
			m_sqlite, 
			"SELECT name, can_read, can_post, last_fetch_time FROM group",
			-1,
			&stmt,
			NULL);
	assert(ret == SQLITE_OK);

	while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
		GroupInfo gi;
		gi.name = (wxChar*) sqlite3_column_text16(stmt, 1);
		gi.can_read = bool(sqlite3_column_int(stmt, 2));
		gi.can_post = bool(sqlite3_column_int(stmt, 3));
		gi.last_fetch = time_t(sqlite3_column_int(stmt, 4));

		result.push_back(gi);
	}
	assert(ret == SQLITE_DONE);
	sqlite3_finalize(stmt);

	return result;
}

void RemarksStorage::AddGroup(std::string group) {

	sqlite3_stmt *stmt;
	int ret = sqlite3_prepare_v2(
			m_sqlite, 
			"DELETE FROM group where name = ?",
			-1,
			&stmt,
			NULL);
	assert(ret == SQLITE_OK);

	ret = sqlite3_bind_text(stmt, 1, group.c_str(), -1, NULL);
	assert(ret == SQLITE_OK);
	ret = sqlite3_step(stmt);
	assert(ret == SQLITE_DONE);
	sqlite3_finalize(stmt);
	
	ret = sqlite3_prepare_v2(
			m_sqlite, 
			"INSERT INTO group VALUES (?, ?, ?, ?)",
			-1,
			&stmt,
			NULL);
	assert(ret == SQLITE_OK);
	ret = sqlite3_bind_text(stmt, 1, group.c_str(), -1, NULL);
	assert(ret == SQLITE_OK);
	ret = sqlite3_bind_int(stmt, 2, 1);
	assert(ret == SQLITE_OK);
	ret = sqlite3_bind_int(stmt, 3, 1);
	assert(ret == SQLITE_OK);
	ret = sqlite3_bind_int(stmt, 4, -1);

	ret = sqlite3_step(stmt);
	assert(ret == SQLITE_DONE);

	sqlite3_finalize(stmt);
}

bool RemarksStorage::Init(wxString &error) {

	bool first_run = false;
	wxString db_path = wxGetHomeDir() + _T("/.szarp/remarks/remarks.db");
	wxString articles_path = wxGetHomeDir() + _T("/.szarp/remarks/articles");

	if (!wxFile::Exists(db_path)) {

		wxString path = wxGetHomeDir() + _T("./szarp");
		if (!create_dir(path, error))
			return false;

		path += _T("/remarks");
		if (!create_dir(path, error))
			return false;

		path += _T("/articles");
		if (!create_dir(path, error))
			return false;

		first_run = true;

	}

	m_maildir_session = vm::create<vm::net::session>();
	m_maildir_store = m_maildir_session->getStore(vm::utility::url(std::string("maildir://localhost") + SC::S2A(articles_path)));
	
	int ret = sqlite3_open16((void*)db_path.c_str(), &m_sqlite);
	if (ret) {
		m_sqlite = NULL;
		error = _("Unable to open database");
		return 1;
	}

	if (!CreateTable()) {
		error = _("Failed to initialize database");
		sqlite3_close(m_sqlite); m_sqlite = NULL;
		return false;
	}

	vm::ref<vm::net::folder> folder = m_maildir_store->getDefaultFolder();
	if (!folder->exists())
		folder->create(1);

	return true;
}


void RemarksStorage::StoreRemark(vm::ref<vm::message> msg) {
	struct tm _tm;
	time_t tref, tsent;

	vm::ref<vm::header> hdr = msg->getHeader();

	vm::charset u8 = vm::charset("utf-8");

	const std::string& graph = hdr->getField("X-DRAW3-REMARK-GRAPH")->getValue().dynamicCast<vmime::text>()->getConvertedText(u8);
	const std::string& window = hdr->getField("X-DRAW3-REMARK-WINDOW")->getValue().dynamicCast<vmime::text>()->getConvertedText(u8);
	const std::string& prefix = hdr->getField("X-DRAW3-REMARK-PREFIX")->getValue().dynamicCast<vmime::text>()->getConvertedText(u8);
	const std::string& msgid = hdr->MessageId()->getValue().dynamicCast<vmime::text>()->getConvertedText(u8);
	const std::string& from = hdr->getField("From")->getValue().dynamicCast<vmime::text>()->getConvertedText(u8);

	const std::string& time_str = hdr->getField("X-DRAW3-REMARK-TIME")->getValue().dynamicCast<vmime::text>()->getConvertedText(u8);
	strptime(time_str.c_str(), "%Y-%m-%d %H:%M:%s", &_tm);
	tref = timegm(&_tm);

	vm::ref<vm::datetime> date = hdr->Date().dynamicCast<vmime::datetime>();
	_tm.tm_year = date->getYear();
	_tm.tm_mon = date->getMonth();
	_tm.tm_mday = date->getDay();
	_tm.tm_hour = date->getHour();
	_tm.tm_min = date->getMinute() / 10 * 10;
	_tm.tm_sec = 0;

	tsent = timegm(&_tm);

	sqlite3_stmt *stmt;
	int ret = sqlite3_prepare_v2(
			m_sqlite, 
			"INSERT INTO remark (graph, window, prefix, msgid, ref_time, from, send_time, title, is_read) "
			"	VALUES ( ?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);",
			-1,
			&stmt,
			NULL);
	assert(ret == SQLITE_OK);

	ret = sqlite3_bind_text(stmt, 1, graph.c_str(), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_text(stmt, 2, window.c_str(), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_text(stmt, 3, prefix.c_str(), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_text(stmt, 4, msgid.c_str(), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_int(stmt, 5, tref);
	assert(ret == 0);
	ret = sqlite3_bind_text(stmt, 6, from.c_str(), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_int(stmt, 7, tsent);
	assert(ret == 0);
	ret = sqlite3_bind_int(stmt, 8, 0);
	assert(ret == 0);
	ret = sqlite3_step(stmt);
	assert(ret == SQLITE_DONE);
	sqlite3_finalize(stmt);

	vm::ref<vm::net::folder> folder = m_maildir_store->getDefaultFolder();
	folder->open(vm::net::folder::MODE_READ_WRITE);
	folder->addMessage(msg);
	folder->close(true);

}

bool RemarksStorage::HasRemarks(const wxString &prefix,
		const wxString &window,
		const wxString &graph,
		const wxDateTime &start_date,
		const wxDateTime &end_date) {

	int ret;
	ret = sqlite3_bind_text16(m_has_remark, 1, static_cast<const void*>(prefix.c_str()), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_text16(m_has_remark, 2, static_cast<const void*>(window.c_str()), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_text16(m_has_remark, 3, static_cast<const void*>(graph.c_str()), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_int(m_has_remark, 4, start_date.GetTicks());
	assert(ret == 0);
	ret = sqlite3_bind_int(m_has_remark, 5, end_date.GetTicks());
	assert(ret == 0);
	ret = sqlite3_step(m_has_remark);
	assert(ret == SQLITE_ROW);
	ret = sqlite3_reset(m_has_remark);
	assert(ret == SQLITE_OK);

	bool result = sqlite3_column_int(m_fetch_remark_info, 1) > 0;

	sqlite3_clear_bindings(m_has_remark);

	return result;
}

std::vector<RemarkInfo> RemarksStorage::GetRemarks(const wxString &prefix,
		const wxString &window,
		const wxString &graph,
		const wxDateTime &start_date,
		const wxDateTime &end_date) {

	std::vector<RemarkInfo> result;

	int ret;
	ret = sqlite3_bind_text16(m_fetch_remark_info, 1, static_cast<const void*>(prefix.c_str()), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_text16(m_fetch_remark_info, 2, static_cast<const void*>(window.c_str()), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_text16(m_fetch_remark_info, 3, static_cast<const void*>(graph.c_str()), -1, NULL);
	assert(ret == 0);
	ret = sqlite3_bind_int(m_fetch_remark_info, 4, start_date.GetTicks());
	assert(ret == 0);
	ret = sqlite3_bind_int(m_fetch_remark_info, 5, end_date.GetTicks());
	assert(ret == 0);

	while ((ret = sqlite3_step(m_fetch_remark_info)) == SQLITE_ROW) {
		RemarkInfo r;
		r.id = (wxChar*) sqlite3_column_text16(m_fetch_remark_info, 1);
		r.title = (wxChar*) sqlite3_column_text16(m_fetch_remark_info, 2);
		r.from = (wxChar*) sqlite3_column_text16(m_fetch_remark_info, 3);
		r.send_time = sqlite3_column_int(m_fetch_remark_info, 4);
		r.is_read = sqlite3_column_int(m_fetch_remark_info, 5) == 1;
		result.push_back(r);
	}
	assert(ret == SQLITE_DONE);
	ret = sqlite3_reset(m_fetch_remark_info);
	assert(ret == SQLITE_OK);
	sqlite3_clear_bindings(m_fetch_remark_info);
	return result;
}

RemarksFrame::RemarksFrame(ConfigManager *config_manager) : 
		m_auth_dialog(NULL), 
		m_cfg_manager(config_manager) {

	wxXmlResource::Get()->LoadFrame(this, NULL, _T("Remarks"));
	SetSize(700, 500);
	SetIcon(szFrame::default_icon);

	wxConfigBase *cfg = wxConfig::Get();
	wxString address, port;
	address = cfg->Read(_T("NNTPServerAddres"), _T("www.szarp.com.pl"));
	port = cfg->Read(_T("NNTPServerPort"), _T("563"));

	m_connection_data.address.Hostname(address);
	m_connection_data.address.Service(port);
}

void RemarksFrame::OnClose(wxCloseEvent &event) {

	if (event.CanVeto() == false) {
		event.Veto();
		Hide();
	} else 
		Destroy();
}

void RemarksFrame::OnFetch(wxCommandEvent &event) {
	if (m_connection_data.username.empty())  {
		if (GetUsernamePassword() == false)
			return;
	}
}

bool RemarksFrame::GetUsernamePassword() {

	if (m_auth_dialog == NULL) 
		m_auth_dialog = new AuthInfoDialog(this);

	int ret = m_auth_dialog->ShowModal();
	if (ret != wxID_OK)
		return false;

	m_connection_data.username = SC::S2A(m_auth_dialog->GetUsername());
	m_connection_data.password = SC::S2A(m_auth_dialog->GetPassword());

	return true;
}

void RemarksFrame::OnRefetchGroupList(wxCommandEvent &event) {
	if (m_connection_data.username.empty())
		return;

	if (GetUsernamePassword() == false)
		return;

}

void NNTPClient::TaskCompleted(NNTPTaskCompleted *tc) {
	bool handled = false;
	for (std::vector<TaskCreator*>::iterator i = m_task_creators.begin();
			i != m_task_creators.end();
			i++) 
		if ((*i)->WaitingForCompletion(tc->m_task->GetTaskId())) {
			handled = true;
			(*i)->TaskCompleted(tc);
			break;
		}

	if (!handled) {
		delete tc->m_task;
		delete tc;
	}
}

TaskCreator::TaskCreator(NNTPClient *client) : m_client(client) {}

bool TaskCreator::WaitingForCompletion(size_t task_id) {
	for (std::vector<size_t>::iterator i = m_pending_tasks.begin();
			i != m_pending_tasks.end();
			++i) 
		if ((*i) == task_id)
			return true;

	return false;
}

void TaskCreator::EnqueTask(NNTPTask *task) {
	m_pending_tasks.push_back(task->GetTaskId());

}

void TaskCreator::TaskCompleted(NNTPTaskCompleted *tc) {
	size_t id = tc->m_task->GetTaskId();
	std::vector<size_t>::iterator i = std::remove(m_pending_tasks.begin(),
			m_pending_tasks.end(), 
			id);

	m_pending_tasks.erase(i, m_pending_tasks.end());

	HandleCompletedTask(tc);
}

BEGIN_EVENT_TABLE(RemarksFrame, szFrame)
	EVT_CLOSE(RemarksFrame::OnClose)
	EVT_MENU(XRCID("FETCH_REMARKS_MENU"), RemarksFrame::OnFetch)
	EVT_MENU(XRCID("REFETCH_GROUP_LIST"), RemarksFrame::OnRefetchGroupList)
END_EVENT_TABLE()

#endif
#endif

