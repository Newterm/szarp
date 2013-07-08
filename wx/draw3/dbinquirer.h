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
 * draw3
 * SZARP
 
 *
 * $Id$
 */
#ifndef __DBINQUIER__
#define __DBINQUIER__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <list>

/**Interface for objects communiating with the database*/
class DBInquirer {
private:
	/**object identification number*/
	InquirerId m_inquirer_id;
protected:
	/**@see DatabaseManager*/
	DatabaseManager* m_database_manager;
	/**queries database
	 * @param @see DatabaseQuery*/
	void QueryDatabase(DatabaseQuery *query);

	void QueryDatabase(std::list<DatabaseQuery*> &qlist);
public: 
	DBInquirer(DatabaseManager *manager);

	/**@return object id*/
	virtual InquirerId GetId() const;

	/**@return current param, queries for values of this param will have higher priority than queries to other params*/
	virtual DrawInfo* GetCurrentDrawInfo();

	/**@return current time, used for queries prioritization*/
	virtual time_t GetCurrentTime();

	/**method is called when response from db is received
	 * @param query object holding response from database, the same instance that was sent to the database via 
	 * @see QueryDatabase method but with proper fields filled*/
	virtual void DatabaseResponse(DatabaseQuery *query);

	/**Removes object from @see DatabaseManager*/
	virtual ~DBInquirer();
};


#endif
