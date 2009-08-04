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
#ifndef __SSUSERDB_H_
#define __SSUSERDB_H_
#include "config.h"
#include <stdio.h>
#include <libxml/tree.h>
#include <libxml/xmlIO.h>
#include <libxml/relaxng.h>
#include <libxml/xpath.h>
#include <libxml/chvalid.h>

#include "liblog.h"
#include "xmlutils.h"
#include "sspackexchange.h"
#include "hwkey.h"

/**Interface for users database*/
class UserDB {
	public:
	/**Checks if given user exists in database
	 * @param user name of the user
	 * @param passwd user's password
	 * @param basedir output param base directory for files synced by user(freed by caller)
	 * @param sync output param, regular expression describing directories from base dir to be synced(freed by caller)
	 * @return true if user with provided creditenials exists, false if not - in that case values
	 * of basedir and sync are not modified*/
	virtual bool FindUser(const char* user, const char* passwd, char** basedir, char **sync) = 0;
	/**Checks if given user has proper hwkey, noexpired account, newest protocol version...
	 * @param protocol protocol number used by client
	 * @param user name of the user
	 * @param passwd user's password
	 * @param key user's hardware key
	 * @param msg output param, used when redirect, and contains proper ip address 
	 * @return auth message. later it will be send's to user see @Message
	 */
	virtual	int CheckUser(int protocol,const char *user, const char *passwd, const char *key, char **msg) = 0;

	virtual ~UserDB();
};


/**Specialization of UserDB working on XML files*/
class XMLUserDB : public UserDB {
private:
	/**Document containing users' database*/
	xmlDocPtr m_document;
	/**xpath query context*/
	xmlXPathContextPtr m_context;

	/**
	 * @param rpc_address	address of RPC server for managing users configuration file
	 */
	XMLUserDB(xmlDocPtr doc, const char *prefix, const char *rpc_address);
	/**Escapes provided string < is substistudted with ;lt and so on..
	 * @param string string to escape
	 * @return escaped string, freed by a caller*/
	char *EscapeString(const char* string);
	const char *hostname;/**<configuration_prefix or `hostname -s`*/
	/**address of RPC server for managing users configuration file*/
	const char *m_rpc;

public:
	/**Loads xml with user database and configuration prefix
	 * @return pointer to XMLUserDB object or NULL if operation did not suceed*/
	static XMLUserDB* LoadXML(const char *path, const char *prefix, const char *rpc);

	virtual bool FindUser(const char* user, const char* passwd, char** basedir, char **sync);
	virtual int CheckUser(int protocol,const char *user, const char *passwd, const char *key, char **msg);
	
	virtual ~XMLUserDB();

};

#endif
