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

#ifndef __ACCESS_H__
#define __ACCESS_H__

#include "uri.h"

class ACL;

/**
 * This class is responsible for checking access for setting param values. It
 * reads configuration file with list of param tree nodes and users which are
 * authorized to change param values. Then it performs PAM based authorization
 * of users. Access is reseted some time after last param modification.
 */
class AccessManager {
	public:
		AccessManager(char *conf_file);
		~AccessManager(void);
		int check(char *user, char *pass, char *method, ParsedURI *uri);
	protected:
		ACL *acl;	/**< Access Control List for params. */
};

#endif

