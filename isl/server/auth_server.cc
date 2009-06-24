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
 * HTTP server - version with basic HTTP authentication and SSL encryption.
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <errno.h>
#include <unistd.h>
#include <liblog.h>
#include <sys/time.h>

#include "auth_server.h"
#include "base64.h"
#include "tokens.h"

/**
 * This method is a replacement for standard ConnectionHandler:do_handle
 * method. It forces HTTP client to perform HTTP Basic 
 * authentication by setting 401 (Unauthorized) error code. Seeing this
 * code, HTTPResponse::response() method sends WWW-Authenthicate header.
 */
int AuthConnectionHandler::do_handle(Connection *conn)
{
	HTTPRequestParser *parser;
	HTTPRequest *req;
	HTTPResponse *res;
	char **toks;
	int tokc = 0;
	char *auth_string;
	struct timeval t1, t2;
	unsigned char *addrt;

	parser = new HTTPRequestParser(conn);
	
	/* parse client's request */
	req = parser->parse(timeout);
	if (req == NULL) 
		goto error;
	/* perform authorization */
	if (req->suggested_code == 200) {
		if (req->auth) {
			/* we expect string of form: 'Basic: user_pass' */
			tokenize_d(req->auth, &toks, &tokc, " ");
			if (tokc < 2) 
				goto noauth;
			if (strcmp(toks[0], "Basic")) 
				goto noauth;
			/* user and password is base64 encoded */
			auth_string = (char *)base64_decode
				((unsigned char *)toks[1]);
			/* decoding error */
			if (auth_string == NULL) {
				tokenize_d(NULL, &toks, &tokc, NULL);
				goto noauth;
			}
			/* auth_string is "user:password" */
			tokenize_d(auth_string, &toks, &tokc, ":");
			free(auth_string);
			/* ask AccessManager for checking user against
			 * password */
			if (acm->check(toks[0], toks[1], req->method, req->uri) == 0)
				req->suggested_code = 401;
			/* free memory */
			tokenize_d(NULL, &toks, &tokc, NULL);
		} else {
noauth:
			if (acm->check(NULL, NULL, req->method, req->uri) == 0)
				req->suggested_code = 401;
		}
	}
	gettimeofday(&t1, NULL);
	/* create response */
	res = new HTTPResponse(conn);
	/* allow setting parameters (third argument is 1) */
	res->response(req, conh, 1);
	gettimeofday(&t2, NULL);
	addrt = (unsigned char *)(&(conn->addr));
	sz_log(2, "%d.%d.%d.%d %s '%s' HTTPS/1.%d %d %ld ms",
		addrt[0], addrt[1], addrt[2], addrt[3], req->method,
		req->unparsed_uri, (req->protocol_version == 11 ? 1 : 0),
		res->code, ((t2.tv_sec - t1.tv_sec) * 1000) + 
		((t2.tv_usec - t1.tv_usec) / 1000));
	delete req;
	delete res;
error:
	delete parser;
	delete conn;
	sz_log(10, "connection closed");
	return 0;
}
