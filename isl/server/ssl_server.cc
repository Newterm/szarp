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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
 
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "liblog.h"

#include "ssl_server.h"
#include "server.h"

/**
 * Private key password.
 */
static const char *password = PASSWORD;

/**
 *  Callback for handing password to SSL library.
 */
static int pem_passwd_cb(char *buf, int size, int rwflag, void *userdata)
{
	strncpy(buf, password, size);
	buf[size - 1] = '\0';
	return(strlen(buf));
}

SSLServer::~SSLServer()
{
	CLEAR(cert);
	CLEAR(key);
	CLEAR(ca_list);
	if (ctx) {
		SSL_CTX_free(ctx);
	}
}

/**
 * Initializes library, reads certificates and keys. 
 * @return 0 on success, -1 on error
 */
int SSLServer::init(void)
{
	SSL_METHOD *meth;
		
	if (is_init) {
		sz_log(1, "Server already initialized");
		return -1;
	}
	sz_log(2, "Initializing SSL connections factory");
	SSL_library_init();
	SSL_load_error_strings();
	signal(SIGPIPE, SIG_IGN);
	meth = (SSL_METHOD*)SSLv23_server_method();
	ctx = SSL_CTX_new(meth);
	if (SSL_CTX_use_certificate_chain_file(ctx, cert) == 0) {
		sz_log(1, "Can't load certificate file %s: %s", cert, 
				(char *)ERR_error_string(ERR_get_error(), NULL));
		goto error;
	}
	password = strdup(pass);
	SSL_CTX_set_default_passwd_cb(ctx, pem_passwd_cb);
	if (!(SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM))) {
		sz_log(1, "Can't read key file");
		goto error;
	}
	if (ca_list) {
		if(!(SSL_CTX_load_verify_locations(ctx, ca_list, 0))) {
			sz_log(1, "Can't read CA list");
			goto error;
		}
	}
	is_init = 1;
	return 0;
error:
	SSL_CTX_free(ctx);
	ctx = NULL;
	return -1;
}

/**
 * Starts SSL server connection on given socket and returns it.
 * @param ch "parent" connection handler
 * @param socket net socket with estabilshed TCP/IP connection
 * @param addr client's IP
 * @return newly created SSL connection, NULL on error
 */
Connection * SSLServer::new_connection(ConnectionHandler *ch, int socket,
	uint32_t addr)
{
	SSLConnection *ssl_c;
	
	if (!is_init) { 
		if (init() < 0)
			return NULL;
	}
	ssl_c = new SSLConnection(ctx, ch, socket, addr);
	if (ssl_c == NULL)
		return NULL;
	if (ssl_c->isOK())
		return ssl_c;
	else
		delete ssl_c;
	return NULL;
}

/**
 * SSL connection constructor - creates SSL connection from given context and on
 * given socket, performs server SSL accept handshake. In case of error sets 
 * flag is_ok to 0, otherwise to 1.
 * @param ctx SSL context
 * @param socket socket with active TCP/IP connection
 */
SSLConnection::SSLConnection(SSL_CTX *ctx, ConnectionHandler *ch, 
		int socket, uint32_t addr) : Connection(ch, socket, addr)
{
	is_ok = 0;
	bio = BIO_new_socket(socket, BIO_CLOSE);
	ssl = SSL_new(ctx);
	SSL_set_bio(ssl, bio, bio);
	if (SSL_accept(ssl) <= 0) {
		sz_log(0, "SSL accept error");
		return;
	}
	is_ok = 1;
}

SSLConnection::~SSLConnection(void) 
{
	int ret;
	if (!is_ok)
		return;
	ret = SSL_shutdown(ssl);
	if (ret == 0) {
		ret = SSL_shutdown(ssl);
	}
	SSL_free(ssl);
	bio = NULL;
	close(sock_fd);
	sock_fd = -1;
}

/**
 * Socket reading wrapper.
 * @param buf buffer to read data to
 * @param size of buffer
 * @return number of bytes read
 */
int SSLConnection::read(void *buffer, int length)
{
	return SSL_read(ssl, buffer, length);
}

/**
 * Socket writing wrapper.
 * @param buf buffer to read data from
 * @param size of data to write
 * @return number of bytes written
 */
int SSLConnection::write(void *buffer, int length)
{
	return SSL_write(ssl, buffer, length);
}
	  
