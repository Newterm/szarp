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
 * SSL classes.
 * $Id$
 */

#ifndef __SSL_VERSION_H__
#define __SSL_VERSION_H__

#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <openssl/ssl.h>
#include <stdarg.h>

#include "server.h"

/**
 * Default private key password (not used, really)
 */
#define PASSWORD "password"

class SSLConnection;

/**
 * Implementation of SSL server (SSL connections factory).
 */
class SSLServer : public ConnectionFactory {
	public :
	/**
	 * SSL server constructor. Initialization of library is performed during
	 * first call to "new_connection()" or "init()" method.
	 * @param cert path to file containing certificate
	 * @param key path to file containing private RSA key
	 * @param ca_list path to file containing CA certificate(s) (may be
	 * NULL)
	 * @param passwd password used to encrypt key
	 */
		SSLServer(char *_cert, char *_key, char *_ca_list, 
				char *_pass) : 
			cert(_cert), key(_key), ca_list(_ca_list), pass(_pass),
			ctx(NULL)
		{ 
			is_init = 0;
		}
		virtual ~SSLServer();
		virtual int init(void);
		virtual Connection *new_connection(ConnectionHandler *ch, 
				int socket, uint32_t addr);
	protected:
		char *cert;	/**< path to file containing certificate */
		char *key;	/**< path to file containing private RSA key */
		char *ca_list;	/**< path to file containing CA certificates */
		char *pass;	/**< password used to encrypt private key */
		SSL_CTX *ctx;	/**< SSL context */
};

/**
 * Single SSL connection.
 */
class SSLConnection : public Connection {
	public:
		SSLConnection(SSL_CTX *ctx, ConnectionHandler *_ch, 
				int socket, uint32_t addr);
		virtual ~SSLConnection(void);
		/**
		 * Checks if connection was properly estabilshed.
		 * @return 1 if connections is OK, otherwise 0
		 */
		int isOK(void)
		{
			return is_ok;
		};
		virtual int read(void *buffer, int length);
		virtual int write(void *buffer, int length);
	protected:
		int is_ok;	/**< Flag - is connections established. */
		SSL *ssl;	/**< SSL connection */
};

#endif

