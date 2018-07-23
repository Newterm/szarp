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
#include <utility>
#include <limits>

#include "cconv.h"
#include "szbase/szbdefines.h"

#include "classes.h"

#include "database.h"
#include "dbinquirer.h"
#include "drawtime.h"
#include "drawobs.h"
#include "cfgmgr.h"
#include "coobs.h"
#include "defcfg.h"
#include "errfrm.h"
#include "dbmgr.h"
#include "parlist.cpp"
#include "user_def_ipk.h"

using namespace SC;

DatabaseManager* ConfigurationFileChangeHandler::database_manager(nullptr);

void
ConfigurationFileChangeHandler::handle(std::wstring file, std::wstring prefix) {
	ConfigurationChangedEvent e(prefix);
	wxPostEvent(database_manager, e);
};


bool
SyncedPrefixSet::Contains(wxString prefix) {
	wxMutexLocker lock(mutex);
	std::set<wxString>::iterator i = prefixes.find(prefix);
	return i != prefixes.end();
}

bool
SyncedPrefixSet::TryAdd(wxString prefix) {
	wxMutexLocker lock(mutex);
	return prefixes.insert(prefix).second;
}

bool
SyncedPrefixSet::Remove(wxString prefix){
	wxMutexLocker lock(mutex);
	std::set<wxString>::iterator i = prefixes.find(prefix);
	if(i != prefixes.end()) {
		prefixes.erase(i);
		return true;
	}
	return false;
}

DEFINE_EVENT_TYPE(DATABASE_RESP)
DEFINE_EVENT_TYPE(CONFIGURATION_CHANGE)
DEFINE_EVENT_TYPE(PARAM_DATA_CHANGED)
wxDEFINE_EVENT(IKS_CONNECTION_FAILED, wxCommandEvent);

BEGIN_EVENT_TABLE(DatabaseManager, wxEvtHandler) 
	EVT_DATABASE_RESP(wxID_ANY, DatabaseManager::OnDatabaseResponse)
	EVT_CONFIGUARION_CHANGE(wxID_ANY, DatabaseManager::OnConfigurationChange)
	EVT_PARAM_DATA_CHANGED(wxID_ANY, DatabaseManager::OnParamDataChanged)
	EVT_COMMAND(wxID_ANY, IKS_CONNECTION_FAILED, DatabaseManager::OnIksConnectionFailed)
END_EVENT_TABLE()

DatabaseResponse::DatabaseResponse(DatabaseQuery *_data) : wxCommandEvent(DATABASE_RESP, wxID_ANY), data(_data)
{}

wxEvent* DatabaseResponse::Clone() const {
	return new DatabaseResponse(*this);
}

DatabaseQuery* DatabaseResponse::GetQuery() {
	return data;
}

DatabaseResponse::~DatabaseResponse()
{}

ConfigurationChangedEvent::ConfigurationChangedEvent(std::wstring prefix) : wxCommandEvent(CONFIGURATION_CHANGE, wxID_ANY), m_prefix(prefix) {}

std::wstring ConfigurationChangedEvent::GetPrefix() {
	return m_prefix;
}

wxEvent* ConfigurationChangedEvent::Clone() const {
	return new ConfigurationChangedEvent(*this);
}

ConfigurationChangedEvent::~ConfigurationChangedEvent() {}

ParamDataChangedEvent::ParamDataChangedEvent(TParam *param) : wxCommandEvent(PARAM_DATA_CHANGED, wxID_ANY), m_param(param) {}

TParam* ParamDataChangedEvent::GetParam() {
	return m_param;
}

wxEvent* ParamDataChangedEvent::Clone() const {
	return new ParamDataChangedEvent(m_param);
}

ParamDataChangedEvent::~ParamDataChangedEvent() {}

DatabaseManager::DatabaseManager(DatabaseQueryQueue *_query_queue, ConfigManager *_config_manager, std::shared_ptr<UserDefinedIPKManager> _ipk_manager) :
	query_queue(_query_queue), config_manager(_config_manager), ipk_manager(_ipk_manager), current_inquirer(-1), free_inquirer_id(1)
{
	ConfigurationFileChangeHandler::database_manager = this;
}

void DatabaseManager::OnDatabaseResponse(DatabaseResponse &response) {

	DatabaseQuery *query = response.GetQuery();

	CheckAndNotifyAboutError(response);

	if (query->type == DatabaseQuery::REMOVE_PARAM) {
		ipk_manager->RemoveUserDefined(query->defined_param.prefix, query->defined_param.p);
		free(query->defined_param.prefix);
		delete query;
	} else if (query->type == DatabaseQuery::STARTING_CONFIG_RELOAD) {
		std::wstring dbprefix(query->reload_prefix.prefix);
		free(query->reload_prefix.prefix);
		delete query;
		config_manager->ConfigurationReadyForLoad(dbprefix.c_str());
	} else {

		IHI i = inquirers.find(query->inquirer_id);
		if (i == inquirers.end()) {
			delete query;
		} else {
			DBInquirer *inquirer = i->second;
			inquirer->DatabaseResponse(query);
		}

	}
}

void DatabaseManager::CheckAndNotifyAboutError(DatabaseResponse &response) {
	DatabaseQuery* q = response.GetQuery();

	if (q->type == DatabaseQuery::GET_DATA) {
		for (std::vector<DatabaseQuery::ValueData::V>::iterator i = q->value_data.vv->begin();
				i != q->value_data.vv->end();
				i++) {
			if (i->ok) 
				continue;
			switch (i->error) {
				case SZBE_CONN_ERROR:
					ErrorFrame::NotifyError(wxString::Format(_("Error in connection with probe server: %s"),
						i->error_str));
					break;
				default:
					ErrorFrame::NotifyError(wxString::Format(_("Database error(%s): %s"),
						q->draw_info->GetName().c_str(),
						i->error_str));
					break;
			}
			free(i->error_str);
		}
	} else if (q->type == DatabaseQuery::SEARCH_DATA) {
		if (!q->search_data.ok) {
			switch (q->search_data.error) {
				case SZBE_SEARCH_TIMEOUT:
					ErrorFrame::NotifyError(wxString::Format(_("Search timed out while seraching for graph: %s"),
						q->draw_info->GetName().c_str(),
						q->search_data.error_str));
					break;
				case SZBE_CONN_ERROR:
					if (std::wstring(L"Connection with probes server not configured") == q->search_data.error_str)
						ErrorFrame::NotifyError(wxString::Format(_("Error in connection with probe server: %s"),
							_("Connection with probes server not configured")));
					else
						ErrorFrame::NotifyError(wxString::Format(_("Error in connection with probe server: %s"),
							q->search_data.error_str));
					break;
				default:
					ErrorFrame::NotifyError(wxString::Format(_("Database error(%s): %s"),
						q->draw_info->GetName().c_str(),
						q->search_data.error_str));
					break;
			}
			free(q->search_data.error_str);
		}
	}
}

void DatabaseManager::SetCurrentInquirer(InquirerId id) {
	current_inquirer = id;
	query_queue->ShufflePriorities();
}
	
void DatabaseManager::InquirerStateChanged() {
	query_queue->ShufflePriorities();
}

InquirerId DatabaseManager::GetCurrentInquirerId() {
	return current_inquirer;
}

DrawInfo * DatabaseManager::GetCurrentDrawInfoForInquirer(InquirerId id) {
	DrawInfo* result;

	IHI i = inquirers.find(id);

	if (i == inquirers.end())
		result = NULL;
	else {
		DBInquirer* di = i->second;
		assert(di != NULL);
		result = di->GetCurrentDrawInfo();
	}

	return result;
}

time_t DatabaseManager::GetCurrentDateForInquirer(InquirerId id) {
	time_t result = -1;

	IHI i = inquirers.find(id);

	if (i == inquirers.end())
		result = -1;
	else {
		wxDateTime t = i->second->GetCurrentTime();
		t.IsValid() ? result = t.GetTicks() : -1;
	}

	return result;
}

void DatabaseManager::RemoveInquirer(InquirerId id) {
	IHI i;
	for (i = inquirers.begin(); i != inquirers.end(); ++i)
		if (i->first == id)
			break;

	if (i != inquirers.end()) {
		inquirers.erase(i);
	}
}

void DatabaseManager::AddInquirer(DBInquirer *inquirer) {
	IHI i = inquirers.find(inquirer->GetId());
	assert(i == inquirers.end());
	inquirers[inquirer->GetId()] = inquirer;
}

InquirerId DatabaseManager::GetNextId() {

	InquirerId id = free_inquirer_id;

	if (free_inquirer_id == std::numeric_limits<InquirerId>::max())
		free_inquirer_id = 1;
	else
		free_inquirer_id++;

	return id;

}

void DatabaseManager::CleanDatabase(DatabaseQuery *query) {
	query_queue->CleanOld(query);
}

void DatabaseManager::QueryDatabase(DatabaseQuery *query) {
	if (query->draw_info) {
		if (reloadingDatabases.Contains(query->draw_info->GetBasePrefix())) {
			delete query;
			return;
		}
	}

	if(query->prefix.empty())
		query->prefix = GetCurrentPrefix().ToStdWstring();
	query_queue->Add(query);
}

void DatabaseManager::QueryDatabase(std::list<DatabaseQuery*> & qlist) {

	std::list<DatabaseQuery *> tmp_queries;

	
	for(std::list<DatabaseQuery*>::iterator i = qlist.begin(); i != qlist.end(); i++){
		if ((*i)->draw_info) {
			if (reloadingDatabases.Contains((*i)->draw_info->GetBasePrefix())) {
				delete (*i);
				continue;
			}
		}
		tmp_queries.push_back(*i);
		if((*i)->prefix.empty())
			(*i)->prefix = GetCurrentPrefix().ToStdWstring();
	}

	query_queue->Add(tmp_queries);
}

void DatabaseManager::InitiateConfigurationReload(wxString prefix) {
	if(!reloadingDatabases.TryAdd(prefix))
		return;

	DatabaseQuery* query = new DatabaseQuery;
	query->type = DatabaseQuery::STARTING_CONFIG_RELOAD;
	query->reload_prefix.prefix = wcsdup(prefix.c_str());
	query->draw_info = NULL;

	query_queue->Add(query);
}

void DatabaseManager::ConfigurationReloadFinished(wxString prefix) {
	reloadingDatabases.Remove(prefix);
}

void DatabaseManager::AddParams(const std::vector<DefinedParam*>& ddi) {

	for (std::vector<DefinedParam*>::const_iterator i = ddi.begin();
			i != ddi.end();
			i++) {

		wxLogInfo(_T("Adding param with prefix: %s"), (*i)->GetBasePrefix().c_str());

		ipk_manager->AddUserDefined((*i)->GetBasePrefix().wc_str(), (*i)->GetIPKParam());

		DatabaseQuery* query = new DatabaseQuery;
		query->type = DatabaseQuery::ADD_PARAM;
		query->defined_param.p = (*i)->GetIPKParam();
		query->defined_param.prefix = wcsdup((*i)->GetBasePrefix());

		query_queue->Add(query);

	}
		
	return;
}

void DatabaseManager::RemoveParams(const std::vector<DefinedParam*>& ddi) {

	for (std::vector<DefinedParam*>::const_iterator i = ddi.begin();
			i != ddi.end();
			i++) {

		wxLogInfo(_T("Removing param with prefix: %s, param: %s"), (*i)->GetBasePrefix().c_str(), (*i)->GetParamName().c_str());

		DatabaseQuery* query = new DatabaseQuery;
		query->type = DatabaseQuery::REMOVE_PARAM;
		query->defined_param.p = (*i)->GetIPKParam();
		query->defined_param.prefix = wcsdup((*i)->GetBasePrefix());
		query_queue->Add(query);
	}
}

void DatabaseManager::OnConfigurationChange(ConfigurationChangedEvent &e) {
	config_manager->ReloadConfiguration(e.GetPrefix());
}

void DatabaseManager::OnParamDataChanged(ParamDataChangedEvent &e) {
	for (IHI i = inquirers.begin(); i != inquirers.end(); i++) {
		i->second->ParamDataChanged(e.GetParam());
	}
}

void DatabaseManager::SetProbersAddresses(const std::map<wxString, std::pair<wxString, wxString> > &addresses) {
	for (std::map<wxString, std::pair<wxString, wxString> >::const_iterator i = addresses.begin();
			i != addresses.end();
			i++) {
		DatabaseQuery* query = new DatabaseQuery;
		query->type = DatabaseQuery::SET_PROBER_ADDRESS;
		query->prefix = i->first.c_str();
		query->prober_address.address = wcsdup(i->second.first.c_str());
		query->prober_address.port = wcsdup(i->second.second.c_str());
		query_queue->Add(query);
	}
}

void DatabaseManager::ChangeObservedParamsRegistration(const std::vector<TParam*>& to_deregister, const std::vector<TParam*>& to_register) {
	DatabaseQuery* query = new DatabaseQuery;

	query->type = DatabaseQuery::REGISTER_OBSERVER;
	query->observer_registration_parameters.observer = this;
	query->observer_registration_parameters.params_to_deregister = new std::vector<TParam*>(to_deregister);
	query->observer_registration_parameters.params_to_register = new std::vector<TParam*>(to_register);
	query->draw_info = NULL;

	query_queue->Add(query);
}

void DatabaseManager::param_data_changed(TParam* param) {
	ParamDataChangedEvent event(param);
	wxPostEvent(this, event);
}

void DatabaseManager::OnIksConnectionFailed(wxCommandEvent &event)
{
	if(frame_controller) {
		wxCommandEvent evt(event);
		wxPostEvent(frame_controller, evt);
	}
}

void DatabaseManager::SetCurrentPrefix(const wxString& prefix)
{
	base_handler->SetCurrentPrefix(prefix);
}

wxString DatabaseManager::GetCurrentPrefix() const
{
	return base_handler->GetCurrentPrefix();
}

void DatabaseManager::AddBaseHandler(const wxString& prefix)
{
	DatabaseQuery* query = new DatabaseQuery;
	query->type = DatabaseQuery::ADD_BASE_PREFIX;
	query->prefix = prefix.ToStdWstring();

	query_queue->Add(query);
}
