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
 * Pawe³ Pa³ucha 2002
 *
 * Access manager - controls access for setting parameters.
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdio.h>
#include <time.h>

#include "liblog.h"

#include "access.h"
#include "server.h"
#include "utf8.h"
#include "pam.h"
#include "tokens.h"

/**
 * Timeout - if it's exceeded, authorization is forced again.
 */
#define ACCESS_TIMEOUT	300

/**
 * Private class to hold ACL for params. It's a list holding nodes' urls and
 * users, that are allowed to modify these nodes.
 */
class ACL {
	public:
		/** Constructor.
		 * @param _uri text representation of param node's URI
		 * @param _user user name
		 * @param _next next list element
		 */
		ACL(char *_uri, char *_user, char* _method, ACL *_next) :
			uri(strdup(_uri)), user(_user ? strdup(_user) : NULL), method(_method ? strdup(_method) : NULL),
			next(_next)
		{ 
			int i = strlen(uri);
			/* No slashes at end... */
			if ((i > 0) && (uri[i-1] == '/'))
				uri[i-1] = 0;
		};
		~ACL()
		{
			if (uri) free (uri);
			if (user) free (user);
			if (method) free (method);
		}
		char *uri;	/**< Text representation of param node's URI. */
		char *user;	/**< User name. */
		char *method;
		ACL *next;	/**< Next list element. */
};

/**
 * @param conf_file configuration file. It should contain lines of format
 * "URI:user", for example "/node1/param2:user_name", which means, that user
 * "user_name" can modify param "/node1/param2". URI's should be UTF-8 encoded.
 * Other lines are ignored.
 */
AccessManager::AccessManager(char *conf_file)
{
	FILE *f;
	int tokc = 0, i;
	char **toks;
#define AC_BUFFER_SIZE	1024
	char buffer[AC_BUFFER_SIZE];
	ACL *tmp;

	/* set last access to "not accessed" */
	acl = NULL;
	/* process config file */
	f = fopen(conf_file, "r");
	if (f == NULL) {
		sz_log(1, "Cannot open access configuration file '%s'",
			conf_file);
		return;
	}
	while (!feof(f)) {
		fgets(buffer, AC_BUFFER_SIZE, f);
		i = strlen(buffer);
		/* strip newlines */
		if ((i > 0) && (buffer[i-1] == '\n'))
			buffer[i-1] = 0;
		tokenize_d(buffer, &toks, &tokc, ":");

		if (tokc < 2)
			continue;
		char *user = toks[1];
		if (!strcmp(toks[1], "*"))
			user = NULL;
		if (tokc == 2) {
			tmp = new ACL(toks[0], user, NULL, acl);
			acl = tmp;
		} else if (tokc == 3) {
			tmp = new ACL(toks[0], user, toks[2], acl);
			acl = tmp;
		}
		/* other lines are ignored without error message */
	}
	/* free memory */
	tokenize_d(NULL, &toks, &tokc, NULL);
	fclose(f);
}

/**
 * Checks if user can modify param. Searches for user and URI in access control
 * list defined in configuration file. When URI and user is find, checks
 * authorization of user with given password (using PAM). 
 * @param user user name
 * @param password user's password
 * @param uri URI of param
 * @return 1 if user is allowed to modify params, 0 if not
 */
int AccessManager::check(char *user, char *pass, char *method, ParsedURI *uri)
{
	char buffer[AC_BUFFER_SIZE];
	int l, i;
	ParsedURI::NodeList *nl;
	ACL *tmp_acl;
	char c;

	/* Make string representation of uri. */
	nl = uri->nodes;
	l = 0;
	for (nl = uri->nodes; nl; nl = nl->next) {
		i = strlen(nl->name);
		if (l + i + 1 >= AC_BUFFER_SIZE) {
			sz_log(1, "AccessManager::check(): uri to long");
			return 0;
		}
		buffer[l] = '/';
		memcpy(buffer + l + 1, nl->name, i);
		l += i + 1;
	}
	buffer[l] = 0;
	/* Search for user and uri in acl. */
	for (tmp_acl = acl; tmp_acl; tmp_acl = tmp_acl->next) if (tmp_acl->user == NULL || (user != NULL && !strcmp(tmp_acl->user, user))) {
		i = strlen(tmp_acl->uri);
		/* Node in rule to deep... */
		if (i > l)
			continue;
		c = buffer[i];
		/* Can we cut an uri to the length of acl's uri ? */
		if ((c != '/') && (c != 0))
			continue;
		/* Cut uri */
		buffer[i] = 0;
		/* Compare */
		if (!cmpURI((unsigned char *)tmp_acl->uri, (unsigned char *)buffer) 
				&& ((tmp_acl->method == NULL) || !strcmp(tmp_acl->method, method))) {
			if (tmp_acl->user == NULL)
				return 1;
			else 
				/* Check user against password using PAM 
				 * Valgrind show double free() if there's no strdup - perhaps libpam
				 * cleans password from memory? But, there's a leak possibility...
				 */
				return pam_check(user, strdup(pass));
		}
		/* Restore uri */
		buffer[i] = c;
	}
	sz_log(3, "Access to '%s' for user '%s' denied.", buffer, user);
	return 0;
}

AccessManager::~AccessManager(void)
{
	ACL *tmp, *p;
	
	for (tmp = acl; tmp; p = tmp, tmp = tmp->next, delete p);
}

