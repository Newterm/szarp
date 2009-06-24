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

#ifndef __REMARKS_H__
#define __REMARKS_H__

#include "config.h"

#ifndef NO_SQLITE3
#ifndef NO_VMIME

#include <vmime/vmime.hpp>

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/list.h>
#include <wx/socket.h>
#endif
#include <string>
#include <vector>
#include <map>

#include <openssl/ssl.h>
#include <sqlite3.h>

#include "szframe.h"

namespace vm = vmime;

class ConfigManager;
class NNTPTaskExecutorState;
class NNTPClient;

struct NNTPConnectionData {
	std::string username;
	std::string password;
	wxIPV4address address;
};

class NNTPStatus {
	int m_x;
	int m_y;
	int m_z;

	public:
	enum TYPE {
		INFORMATIVE = 1,
		COMMAND_OK = 2,
		COMMAND_OK_CONTINUE = 3,
		COMMAND_COULD_NOT_BE_PERFORMED = 4,
		COMMAND_INCORRECT = 5,
	};

	NNTPStatus();

	const wxChar* GetDescription() const;

	TYPE GetType() const;
	int GetValue() const;

	bool Parse(const std::string& response);
};

class NNTPSSLConnection : public wxEvtHandler {
	static const int input_buffer_size;

	wxSocketClient *m_socket;
	SSL *m_ssl;
	SSL_CTX *m_ssl_ctx;

	char* m_output_buffer;
	int m_output_buffer_size;
	int m_output_buffer_len;

	char* m_input_buffer;
	int m_input_buffer_size;
	int m_input_buffer_len;

	wxEvtHandler *m_handler;

	enum STATE {
		NOT_CONNECTED,
		CONNECTING,
		CONNECTING_SSL,
		READ_BLOCKED_ON_WRITE,
		WRITE_BLOCKED_ON_READ,
		IDLE
	} m_state;

	void SocketEvent(wxSocketEvent &event);
	void ConnectionTerminated();
	void Cleanup();
	void Error(const std::string& cause);
	void SocketConnected();
	void WriteData();
	void ReadData();
	void PostReadData();
	void ConnectSSL();
public:
	NNTPSSLConnection(wxEvtHandler *handler);
	bool IsConnected() const;
	bool Connect(wxIPV4address address);
	bool Query(const std::string &query);
	~NNTPSSLConnection();

	DECLARE_EVENT_TABLE()

};

class NNTPConnectionEvent : public wxCommandEvent {
public:
	enum Type {
		CONNECTION_LOST,
		CONNECTION_FAILED,
		CONNECTION_READY,
		DATA_WRITTEN,
		DATA_READ
	};
private:
	Type m_type;

	std::string m_data;
public:
	NNTPConnectionEvent(Type, const std::string& data = "");

	virtual wxEvent* Clone() const;

	Type GetEventType() const;

	const std::string& GetData() const;
};

class NNTPTask {
	static size_t free_task_id;
private:
	size_t m_task_id;
public:
	NNTPTask();
	size_t GetTaskId();
	virtual ~NNTPTask() {}
};

WX_DECLARE_LIST(NNTPTask, NNTPTaskQueue);

class NNTPTaskError {
	NNTPStatus m_nntp_error;
	wxString m_network_error;
	bool m_is_nntp_error;
public:
	void SetNetworkError(wxString error);
	void SetNNTPError(const NNTPStatus& m_nntp_error);

};

struct NNTPTaskCompleted {
	NNTPTask* m_task;
	NNTPTaskError m_error;
	bool m_is_error;
};

struct RemarkInfo {
	wxString id;
	wxString title;
	wxString from;
	bool is_read;
	time_t send_time;
};

struct GroupInfo {
	wxString name;
	wxString config;
	bool can_read;
	bool can_post;
	time_t last_fetch;
};

class PostMessageTask : public NNTPTask {
	vm::ref<vm::message> m_msg;
	std::string m_group;
public:
	PostMessageTask(vm::ref<vm::message> msg, const std::string& group);
	vm::ref<vm::message> GetMessage() const;
	const std::string& GetGroup() const;
	virtual ~PostMessageTask();
};

class FetchMessageTask : public NNTPTask {
	std::string m_msg_id;
	vm::ref<vm::message> m_article;
public:
	FetchMessageTask(const std::string& msg_id);
	const std::string& GetMsgId() const;
	void SetArticle(vm::ref<vm::message> article);
	vm::ref<vm::message> GetArticle();
};

class FetchNewMessagesTask : public NNTPTask {
	time_t m_start_time;
	std::string m_group;
	std::vector<std::string> m_ids;
public:
	FetchNewMessagesTask(time_t start_time, const std::string &group);
	const std::string& GetGroup() const;
	void SetIds(const std::vector<std::string>& ids);
	time_t GetStartTime() const;

};

class GetGroupsTask : public NNTPTask {
	std::vector<std::string> m_groups;
public:
	void SetGroups(const std::vector<std::string>& groups);
	const std::vector<std::string>& GetGroups();
};

class NNTPTaskExecutor : public wxEvtHandler {
	NNTPConnectionData m_connection_data;
	NNTPClient *m_client;
	NNTPSSLConnection m_connection;
	NNTPTaskQueue m_queue;
	WX_DECLARE_STRING_HASH_MAP(NNTPTaskExecutorState* , StatesHash);
	StatesHash m_states;
	NNTPTaskExecutorState *m_state;	
	void OnConnectionEvent(NNTPConnectionEvent &event);

public:
	NNTPTaskExecutor(NNTPConnectionData &conn_data, NNTPClient *client);

	bool CheckStatusCode(const NNTPConnectionEvent &event, NNTPStatus::TYPE type);

	void HandleNNTPError(NNTPStatus& status);

	void SetConnData(NNTPConnectionData &conn_data);

	void EnterState(const wxString& state);

	void TaskCompleted(NNTPTaskCompleted *nntp_completed);

	void ExecuteTask(NNTPTask *task);

	~NNTPTaskExecutor();

	DECLARE_EVENT_TABLE();
};

class NNTPTaskExecutorState { 
protected:
	bool m_awaiting_status;

	NNTPConnectionData &m_connection_data;
	NNTPTaskQueue &m_queue; 
	NNTPSSLConnection &m_connection;
	NNTPTaskExecutor &m_executor;

	void DefaultEventHandler(const NNTPConnectionEvent &event);

	virtual void StatusRead(NNTPStatus &stats, const std::string &line) = 0;

	virtual void LineRead(const std::string& line) = 0;

	virtual void StateEntered() = 0;

public:
	NNTPTaskExecutorState(NNTPConnectionData &data,
			NNTPTaskQueue &queue,
			NNTPSSLConnection &m_connection,
			NNTPTaskExecutor& client);

	virtual void HandleConnectionEvent(const NNTPConnectionEvent &event);
	virtual void Enter();
	virtual void TaskReceived();

	virtual ~NNTPTaskExecutorState();
};

class NNTPConnecting : public NNTPTaskExecutorState {
	int m_failed_connects;

	virtual void LineRead(const std::string &line);
	virtual void StatusRead(NNTPStatus &stats, const std::string &line);
	virtual void StateEntered();
			
public:
	NNTPConnecting(NNTPConnectionData &data,
			NNTPTaskQueue &queue,
			NNTPSSLConnection &m_connection,
			NNTPTaskExecutor& client);

	virtual void HandleConnectionEvent(const NNTPConnectionEvent &event);
	virtual void Enter();
};

class NNTPAuthenticating : public NNTPTaskExecutorState {
protected:
	virtual void LineRead(const std::string &line);
	virtual void StatusRead(NNTPStatus &stats, const std::string &line);
	virtual void StateEntered();
public:
	NNTPAuthenticating(NNTPConnectionData &data,
			NNTPTaskQueue &queue,
			NNTPSSLConnection &m_connection,
			NNTPTaskExecutor& client);

};

class NNTPReady : public NNTPTaskExecutorState {
protected:
	virtual void LineRead(const std::string &line);
	virtual void StatusRead(NNTPStatus &stats, const std::string &line);
	virtual void StateEntered();
	void DispatchTask();
public:
	NNTPReady(NNTPConnectionData & data,
			NNTPTaskQueue &queue,
			NNTPSSLConnection &m_connection,
			NNTPTaskExecutor& client);

	virtual void TaskReceived();

};

class NNTPFetchingArticle : public NNTPTaskExecutorState {
	std::string m_article;
protected:
	virtual void LineRead(const std::string &line);
	virtual void StatusRead(NNTPStatus &stats, const std::string &line);
	virtual void StateEntered();
public:
	NNTPFetchingArticle(NNTPConnectionData & data,
			NNTPTaskQueue &queue,
			NNTPSSLConnection &m_connection,
			NNTPTaskExecutor& client);

};

class NNTPPostingArticle : public NNTPTaskExecutorState {
	enum { CHANGING_GROUP,
		REQUESTING_POST,
		POSTING } m_substate;	
				
	std::vector<std::string> m_ids;
protected:
	virtual void LineRead(const std::string &line);
	virtual void StatusRead(NNTPStatus &stats, const std::string &line);
	virtual void StateEntered();
public:
	NNTPPostingArticle(NNTPConnectionData & data,
			NNTPTaskQueue &queue,
			NNTPSSLConnection &m_connection,
			NNTPTaskExecutor& client);
};

class NNTPFetchingGroups : public NNTPTaskExecutorState {
	std::vector<std::string> m_groups;
protected:
	virtual void LineRead(const std::string &line);
	virtual void StatusRead(NNTPStatus &stats, const std::string &line);
	virtual void StateEntered();
public:
	NNTPFetchingGroups(NNTPConnectionData & data,
			NNTPTaskQueue &queue,
			NNTPSSLConnection &m_connection,
			NNTPTaskExecutor& client);

};

class NNTPFetchingNewMessages : public NNTPTaskExecutorState {
	std::vector<std::string> m_ids;
protected:
	virtual void LineRead(const std::string &line);
	virtual void StatusRead(NNTPStatus &stats, const std::string &line);
	virtual void StateEntered();
public:
	NNTPFetchingNewMessages(NNTPConnectionData & data,
			NNTPTaskQueue &queue,
			NNTPSSLConnection &m_connection,
			NNTPTaskExecutor& client);

};

class NNTPClient;

class TaskCreator {
	NNTPClient *m_client;
	std::vector<size_t> m_pending_tasks;
protected:
	void EnqueTask(NNTPTask *task);
public:
	TaskCreator(NNTPClient *client);

	bool WaitingForCompletion(size_t task_id);

	void TaskCompleted(NNTPTaskCompleted* task);

	virtual void HandleCompletedTask(NNTPTaskCompleted *task) = 0;

	virtual ~TaskCreator() {}
};

class RemarksFrame;

class NNTPClient {
	NNTPTaskExecutor *m_executor;
	RemarksFrame *m_frame;
	wxArrayString m_prefixes;
	NNTPConnectionData m_data;
	std::vector<TaskCreator*> m_task_creators;
	bool m_initialized;
public:
	void TaskCompleted(NNTPTaskCompleted *task);
	void RegisterTaskCreator(TaskCreator *tc);
	void DeregisterTaskCreator(TaskCreator *tc);
	void PerformTask(NNTPTask *task);
};

class RemarksStorage {
	sqlite3* m_sqlite;
	sqlite3_stmt* m_fetch_remark_info;
	sqlite3_stmt* m_has_remark;
	
	bool Init(wxString &error);
	bool CreateTable();
	void PrepareQuery();

	RemarksStorage();
	~RemarksStorage();

	vm::ref<vm::net::store> m_maildir_store;
	vm::ref<vm::net::session> m_maildir_session;

	static RemarksStorage *_instance;
public:
	void StoreRemark(vm::ref<vm::message> msg);

	std::vector<RemarkInfo> GetRemarks(const wxString& prefix,
			const wxString& window,
			const wxString& graph,
			const wxDateTime &start_date,
			const wxDateTime &end_date);

	bool HasRemarks(const wxString& prefix,
			const wxString& window,
			const wxString& graph,
			const wxDateTime &start_date,
			const wxDateTime &end_date);

	std::vector<GroupInfo> GetGroups();

	void AddGroup(std::string group);

	vm::ref<vm::message> GetRemark(const wxString& msg_id);

	static bool GetStorage(RemarksStorage **storage, wxString &error);

	static void Destroy();
};

class AuthInfoDialog;

class RemarksFrame : public wxFrame {
	AuthInfoDialog* m_auth_dialog;
	ConfigManager *m_cfg_manager;
	RemarksStorage *m_storage;
	NNTPConnectionData m_connection_data;
	void OnFetch(wxCommandEvent &event);
	void OnClose(wxCloseEvent &event);
	bool GetUsernamePassword();
	void OnRefetchGroupList(wxCommandEvent &event);
public:	
	RemarksFrame(ConfigManager *manager);
	DECLARE_EVENT_TABLE()
};


#endif
#endif

#endif
