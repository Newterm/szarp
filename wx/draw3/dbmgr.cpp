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

BEGIN_EVENT_TABLE(DatabaseManager, wxEvtHandler) 
	EVT_DATABASE_RESP(wxID_ANY, DatabaseManager::OnDatabaseResponse)
	EVT_CONFIGUARION_CHANGE(wxID_ANY, DatabaseManager::OnConfigurationChange)
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

DatabaseManager::DatabaseManager(DatabaseQueryQueue *_query_queue, ConfigManager *_config_manager) :
	query_queue(_query_queue), config_manager(_config_manager), current_inquirer(-1), free_inquirer_id(1)
{}

void DatabaseManager::OnDatabaseResponse(DatabaseResponse &response) {

	DatabaseQuery *query = response.GetQuery();

	CheckAndNotifyAboutError(response);

	if (query->type == DatabaseQuery::STARTING_CONFIG_RELOAD) {
		std::wstring dbprefix(query->reload_prefix.prefix);
		free(query->reload_prefix.prefix);
		delete query;

		config_manager->ConfigurationReadyForLoad(dbprefix.c_str());
		return;

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

	if (q->type == DatabaseQuery::GET_DATA)
		for (std::vector<DatabaseQuery::ValueData::V>::iterator i = q->value_data.vv->begin();
				i != q->value_data.vv->end();
				i++) {
			if (i->ok) 
				continue;
			ErrorFrame::NotifyError(wxString::Format(_("Definable param error(%s): %s"),
				q->draw_info->GetName().c_str(),
				i->error));
			free(i->error);
		}
	else if (q->type == DatabaseQuery::SEARCH_DATA) {
		if (!q->search_data.ok) {
			ErrorFrame::NotifyError(wxString::Format(_("Definable param error(%s): %s"),
				q->draw_info->GetName().c_str(),
				q->search_data.error));
			free(q->search_data.error);
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
	time_t result;

	IHI i = inquirers.find(id);

	if (i == inquirers.end())
		result = -1;
	else
		result = i->second->GetCurrentTime();

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

		IPKContainer *ic = IPKContainer::GetObject();
		ic->AddExtraParam((*i)->GetBasePrefix().c_str(), (*i)->GetIPKParam());

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

		wxLogInfo(_T("Removing param with prefix: %s"), (*i)->GetBasePrefix().c_str());

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
