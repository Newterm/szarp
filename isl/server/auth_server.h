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
 * HTTP server - version with basic HTTP authentication.
 * $Id$
 */

#ifndef __AUTH_SERVER_H__
#define __AUTH_SERVER_H__

#include "server.h"
#include "ssl_server.h"
#include "access.h"
#include "config_load.h"

/**
 * This class is replacement for standard ConnectionHandler class. It performs 
 * HTTP Basic Authentication for connection and uses SSL for encryption.
 */
class AuthConnectionHandler : public ConnectionHandler {
	public:
		/**
		 * Creates new connection handler with default connection's
		 * timeout.
		 * @param _conh connection's content handler
		 * @param _ssl ssl server to manage connection
		 * @param _acm params access manager
		 */
		AuthConnectionHandler(AbstractContentHandler *_conh, 
				ConnectionFactory *_conf,
				AccessManager *_acm) :
			ConnectionHandler(_conh, _conf), acm(_acm)
		{ };
		/**
		 * @param _conh connection's content handler
		 * @param _timeout connection's timeout
		 * @param _ssl ssl server to manage connection
		 * @param _acm params access manager
		 */
		AuthConnectionHandler(AbstractContentHandler *_conh, int _timeout,
				ConnectionFactory *_conf, AccessManager *_acm) :
			ConnectionHandler(_conh, _conf),
			acm(_acm)
		{ };
		virtual int do_handle(Connection *conn);
	private:
		AccessManager *acm;	/**< access manager provides user 
			authentication and params access control */
};

#endif

