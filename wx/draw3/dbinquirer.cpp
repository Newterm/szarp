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

#include "szhlpctrl.h"

#include "ids.h"
#include "classes.h"

#include "coobs.h"
#include "cfgmgr.h"
#include "defcfg.h"
#include "database.h"
#include "dbinquirer.h"
#include "dbmgr.h"

DBInquirer::DBInquirer(DatabaseManager *manager) {
	m_database_manager = manager;
	m_inquirer_id = manager->GetNextId();
	manager->AddInquirer(this);
}

InquirerId DBInquirer::GetId() const {
	return m_inquirer_id;
}

void DBInquirer::QueryDatabase(DatabaseQuery *query) {
	query->inquirer_id = m_inquirer_id;
	m_database_manager->QueryDatabase(query);
}

void DBInquirer::QueryDatabase(std::list<DatabaseQuery*> &qlist){
	for(std::list<DatabaseQuery*>::iterator i = qlist.begin(); i != qlist.end(); i++){
		(*i)->inquirer_id = m_inquirer_id;
	}
	m_database_manager->QueryDatabase(qlist);
}

DBInquirer::~DBInquirer() {
	m_database_manager->RemoveInquirer(m_inquirer_id);
}


