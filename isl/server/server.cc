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
 * HTTP server.
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <malloc.h>
#include <errno.h>
#include <time.h>
#include <openssl/err.h>
#include <assert.h>
#include <fnmatch.h>

#include "liblog.h"

#include "server.h"
#include "config_load.h"
#include "ssl_server.h"
#include "auth_server.h"
#include "convuri.h"
#include "tokens.h"

#include <stdexcept>

/** Server version */
#define SERVER_VERSION "$Revision: 6290 $ $Date: 2008-12-29 13:09:10 +0100 (pon, 29.12.2008) $"

/**
 * Starts server. This method binds socket to all local address and port
 * given by class constructor. 
 * @return 0 on success, -1 on error
 */
int Server::start(void) 
{
	struct sockaddr_in sin;
	struct sockaddr_in cin;
	struct sockaddr_in sname;
	unsigned int cinlen, snamelen;;
	int on = 1, socknum, nsocknum;

	socknum = socket(AF_INET, SOCK_STREAM, 0);
	if (socknum < 0) {
		sz_log(0, "cannot create socket, errno %d", errno);
		return -1;
	}
	if (setsockopt(socknum, SOL_SOCKET, SO_REUSEADDR,
          &on, sizeof(on)) < 0) { 
		close(socknum);
		sz_log(0, "cannot set socket option, errno %d", errno);
		return -1;
	}
                                                                
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	if (bind(socknum, (struct sockaddr *)&sin, sizeof(sin))) {
		close(socknum);
		sz_log(0, "bind() error, errno %d", errno);
		return -1;
	}
	
	 snamelen = sizeof(sname);
	 if (getsockname(socknum, (struct sockaddr *)&sname, &snamelen)) {
		close(socknum);
		sz_log(0, "getsockname() error, errno %d", errno);
		return -1;
	 }
	 switch(sname.sin_family) {
		 case AF_INET:
			sz_log(2, "starting server on port %d", (int) 
					ntohs(((struct sockaddr_in *)
					&sname)->sin_port));
			break;
		default:
			close(socknum);
			sz_log(0, "unknown address family %d", sname.sin_family);
			return -1;
	 }

 	if (listen(socknum, 1) < 0) {
		close(socknum);
		sz_log(0, "listen() error, errno %d", errno);
		return -1;
	}

	cinlen = sizeof(cin);
	while (1) {
		nsocknum = accept(socknum, (struct sockaddr *)&cin, &cinlen);
		if (accept < 0) {
			sz_log(0, "accept() error, errno %d", errno);
		} else {
			sz_log(10, "connection accepted!");
			ch->handle(nsocknum, cin.sin_addr.s_addr);
		}
	}
}

ConnectionHandler::~ConnectionHandler(void) 
{
	if (allowed_ip)
		free(allowed_ip);
}

void ConnectionHandler::set_allowed_ip(char *ip_list)
{
	if (allowed_ip)
		free(allowed_ip);
	if (ip_list) {
		allowed_ip = strdup(ip_list);
	} else {
		allowed_ip = NULL;
	}
}

int ConnectionHandler::is_allowed(uint32_t addr)
{
	if (allowed_ip == NULL)
		return 1;
#ifndef FNM_EXTMATCH
	return !fnmatch(allowed_ip, inet_ntoa(*(struct in_addr *)&addr), 0);
#else
	return !fnmatch(allowed_ip, inet_ntoa(*(struct in_addr *)&addr), 
			FNM_EXTMATCH);
#endif
}


/**
 * @param _ch parent connection handler
 * @param socket_num TCP/IP connection's socket number
 * @param addr client's IP
 */
Connection::Connection(ConnectionHandler *_ch, int socket_num,
	uint32_t _addr) : 
		ch(_ch), sock_fd(socket_num), addr(_addr)
{
	bio = BIO_new_socket(sock_fd, BIO_CLOSE);
}

/**
 * Socket reading wrapper.
 * @param buf buffer to read data to
 * @param size of buffer
 * @return number of bytes read
 */
int Connection::read(void *buffer, int length)
{
	return BIO_read(bio, buffer, length);
}

/**
 * Socket writing wrapper.
 * @param buf buffer to read data from
 * @param size of data to write
 * @return number of bytes written
 */
int Connection::write(void *buffer, int length)
{
	int ret;
	ret = BIO_write(bio, buffer, length);
	BIO_flush(bio);
	return ret;
}

Connection::~Connection(void)
{
	if (bio) {
		BIO_flush(bio);
		BIO_free(bio);
	}
	if (sock_fd >= 0)
		close(sock_fd);
}

/**
 * Handles connection.
 * @param sock_num file descriptor of connection's socket
 * @param addr client's IP, for information purposes
 * @return -1 on error, 0 on success
 */
int ConnectionHandler::handle(int sock_num, uint32_t addr) 
{
	Connection *conn;

	if (!is_allowed(addr)) {
		sz_log(0, "Connection from address %s refused",
				inet_ntoa(*(struct in_addr *)&addr));
		close(sock_num);
		return -1;
	}
	/* create new connection */
	conn = conf->new_connection(this, sock_num, addr);
	if (conn == NULL) {
		sz_log(1, "Cannot create connection");
		close(sock_num);
		return -1;
	} else {
		/* handle connection */
		sz_log(10, "ConnectionHandler::handle(): about to start new connection");
		conn->ch->do_handle(conn);
		close(sock_num);
		return 0;
	}
}

ConnectionFactory::ConnectionFactory()
	: is_init(0)
{
}

int ConnectionFactory::init(void)
{
	if (is_init) {
		sz_log(1, "Server already initialized");
		return -1;
	}
	is_init = 1;
	return 0;
}

Connection *ConnectionFactory::new_connection(ConnectionHandler *ch,
		int socket, uint32_t addr)
{
	Connection *conn;
	conn = new Connection(ch, socket, addr);
	assert (conn != NULL);
	return conn;
}

/**
 * Actual connection handling. Parses HTTP request, constructs and sends
 * response, then closes connection.
 * @param conn connection to handle
 * @return 0 on success, -1 on error
 */
int ConnectionHandler::do_handle(Connection *conn)
{
	HTTPRequestParser *parser;
	HTTPRequest *req;
	HTTPResponse *res;
	struct timeval t1, t2;
	unsigned char *addrt;
	
	parser = new HTTPRequestParser(conn);
	req = parser->parse(timeout);
	gettimeofday(&t1, NULL);
	if (req == NULL) 
		goto error;
	res = new HTTPResponse(conn);
	res->response(req, conh);
	gettimeofday(&t2, NULL);
	addrt = (unsigned char *)(&(conn->addr));
	sz_log(10, "%d.%d.%d.%d %s '%s' HTTP/1.%d %d %ld ms",
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
	return -1;
}

HTTPRequest::~HTTPRequest(void)
{
	CLEAR(method);
	delete uri;
	CLEAR(unparsed_uri);
	CLEAR(content);
	CLEAR(auth);
}

/**
 * Read and parse request from stream given to class constructor.
 * @param connection timeout, if < 0, timeout is infinitive
 * @return parsed request, NULL on reading error
 */
HTTPRequest * HTTPRequestParser::parse(int timeout)
{
	HTTPRequest *req;
#define BUF_SIZE HTTP_MAX_REQUEST_LEN + 1
	char buffer[BUF_SIZE];

	int fd;
	fd_set rfds;
	struct timeval tv, *tvptr;
	sigset_t mask;
	int retval;
	int br = 0; /* number of bytes read */

	req = new HTTPRequest();
	fd = conn->get_fd();

	if (timeout < 0)
		tvptr = NULL;
	else {
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		tvptr = &tv;
	}
	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigaddset(&mask, SIGCHLD);
	sigaddset(&mask, SIGPIPE);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	while (1) {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		/* Wait up to timeout seconds. */

		retval = select(fd+1, &rfds, NULL, NULL, tvptr);
		
		if (!retval) {
			/* Timeout ! */
			sz_log(10, "client timeout");
			delete req;
			return NULL;
		}
		if (retval < 0) {
			sz_log(4, "select error(), errno %d", errno);
			delete req;
			return NULL;
		}
		retval = conn->read(buffer+br, BUF_SIZE-br-1);
		sz_log(10, "READ: %d\n", retval);
		if (retval <= 0) {
			sz_log(10, "read() <= 0 (%d)", retval);
			if (retval < 0) {
				if ((retval = ERR_get_error()))
					sz_log(4, "SSL ERROR: %s ",
						ERR_error_string
						    (retval, NULL));
			}
			delete req;
			return NULL;
		}
		br += retval;
		buffer[br] = 0;
		if (br >= 4) {
			if (strstr(buffer, "\r\n\r\n")) {
				/* we have all request so we can parse it */
				break;
			}
		}
	}
	if (br >= BUF_SIZE - 1) {
		req->suggested_code = 413; /* Request Entity To Large */
	}
	/* parse request */
	req->parse(buffer);
	if ((req->suggested_code == 200) && (req->content_length > 0)) {
		/* we should have some content */
		int content_read = 0;
		char* c = strstr(buffer, "\r\n\r\n");
		if (c == NULL) {
			/* something is very bad - can't find end of request
			 */
			req->suggested_code = 400;
			return req;
		}
		content_read = (buffer + br - c) - 4;
		sz_log(10, "Searching for content, length is %d, read %d\n", req->content_length,
				content_read);
		if ((br - content_read + req->content_length) > BUF_SIZE) {
			/* don't have space for such a big request */
			req->suggested_code = 413;
			return req;
		}
		if (content_read < req->content_length) while (1) {
			FD_ZERO(&rfds);
			FD_SET(fd, &rfds);
			/* Wait up to timeout seconds. */
			retval = select(fd+1, &rfds, NULL, NULL, tvptr);
			if (!retval) {
				/* Timeout ! */
				sz_log(10, "client timeout in content transfer");
				delete req;
				return NULL;
			}
			if (retval < 0) {
				sz_log(4, "select error() 2, errno %d", errno);
				delete req;
				return NULL;
			}
			retval = conn->read(buffer+br, 
					req->content_length - content_read);
			if (retval <= 0) {
				delete req;
				return NULL;
			}
			content_read += retval;
			br += retval;
			if (content_read >= req->content_length) {
				buffer[br] = 0;
				break;
			}
		}
		if (content_read >= req->content_length) {
			/* now, parse content */
			req->parse_content(buffer);
			return req;
		}
	}
	return req;
}

HTTPRequestParser::~HTTPRequestParser()
{
}


/**
 * Parses client request given as string and fills in class attributes.
 * @param str client request
 */
void HTTPRequest::parse(char *str)
{
	char **lines;
	int ln = 0;
	char **toks;
	int tokc = 0;
	int i;
	char *c;

	sz_log(10, "Parsing request:\n--\n%s\n--", str);
	tokenize_d(str, &lines, &ln, "\r\n");
	if (ln < 0) {
		suggested_code = 400;
		goto free_lines;
	}
	tokenize_d(lines[0], &toks, &tokc, " \t");
	if (tokc != 3) {
		suggested_code = 400;
		goto free_lines;
	}
	method = (char *) xmlStrdup(BAD_CAST toks[0]);
	if (strcmp(method, "GET") && strcmp(method, "POST") &&
			strcmp(method, "PUT")) {
		suggested_code = 501;
		goto free_lines;
	}
	c = convURI(toks[1]);
	unparsed_uri = (char *) xmlStrdup(BAD_CAST c);
	uri = new ParsedURI(c);
	free(c);
	if (!strcmp(toks[2],"HTTP/1.0"))
		protocol_version = 10;
	else if (!strcmp(toks[2],"HTTP/1.1")) {
		protocol_version = 11;
	} else {
		suggested_code = 505;
		goto free_lines;
	}
	for (i = 1; i < ln; i++) {
		tokenize_d(lines[i], &toks, &tokc, ":");
		if (tokc < 1)
			break;
		if (!strcmp(toks[0], "Accept")) {
			if (strstr(toks[1],"text/xml"))
				allow_xml = 1;
			if (strstr(toks[1],"image/svg+xml"))
				allow_svg = 1;
		} else if (!strcmp(toks[0], "Authorization")) {
			auth = (char *) xmlStrdup(BAD_CAST toks[1]);
		} else if (!strcmp(toks[0], "Content-Length")) {
			content_length = atoi(toks[1]);
			if (content_length <= 0) {
				suggested_code = 400; /* Bad Request */
				break;
			} else if (content_length > HTTP_MAX_REQUEST_LEN) {
				suggested_code = 413; /* Request Entity To Large */
				break;
			}
		} else if (!strcmp(toks[0], "Transfer-Encoding")) {
			if (strcmp(toks[1], "identity")) {
				suggested_code = 501; /* Not Implemented */
				break;
			}
		}
	}
	if (tokc > 0) {
		for (i = 0; i < tokc; i++)
			free(toks[i]);
		free(toks);
	}
free_lines:
	if (ln > 0) {
		for (i = 0; i < ln; i++)
			free(lines[i]);
		free(lines);
	}
}

void HTTPRequest::parse_content(char *str)
{
	char *c;
	int len;
	
	assert (suggested_code == 200);
	assert (str != NULL);
	assert (content == NULL);
	sz_log(10, "Searching for content in:\n--\n%s\n--", str);
	/* Content is something that starts after empty line */
	c = strstr(str, "\r\n\r\n");
	if (c == NULL) {
		len = 0;
	} else {
		content = (char *) xmlStrdup(BAD_CAST c + 4);
		len = strlen(content);
	}
	if ((len != content_length) && (len != content_length - 1)) {
		if (content_length <= 0) {
			suggested_code = 411; /* Length Required */
		} else {
			suggested_code = 400; /* Bad Request */
		}
	}
}

HTTPResponse::~HTTPResponse(void)
{
	CLEAR(date);
	CLEAR(exp_date);
	CLEAR(content_type);
	CLEAR(content_coding);
	CLEAR(location);
	CLEAR(content);
}

/**
 * Sends response for given request. Writes response to stream given to
 * constructor.
 * @param req HTTP request to reply to
 * @param conh content handler, responsible for creating response content
 * @param put 0 if setting param value is forbidden, otherwise 1 
 * @return 0 on success, -1 on error
 */
int HTTPResponse::response(HTTPRequest *req, AbstractContentHandler *conh, int put)
{
#define RES_BUFFER_SIZE	1024
	char buffer[RES_BUFFER_SIZE];
	
	buffer[RES_BUFFER_SIZE-1] = 0;
	if (req->suggested_code == 401) {
		/* Authorization failed */
		snprintf(buffer, RES_BUFFER_SIZE-1, "\
HTTP/1.%d %d Error\r\n\
Server: ISL HTTP server %s\r\n\
WWW-Authenticate: Basic realm=\"SZARP\"\r\n\
Connection: Close\r\n\
\r\n\
%d Error",
			1, req->suggested_code,
			SERVER_VERSION, req->suggested_code);
		conn->write(buffer, strlen(buffer));
		return 0;
	}
	
	if (req->suggested_code != 200) {
		snprintf(buffer, RES_BUFFER_SIZE-1, "\
HTTP/1.%d %d Error\r\n\
Server: ISL HTTP server %s\r\n\
Connection: Close\r\n\
\r\n\
%d Error",
			1, req->suggested_code,
			SERVER_VERSION, req->suggested_code);
		conn->write(buffer, strlen(buffer));
		return 0;
	}
	
	if (put && (req->uri->getOption("put"))) {
		if (conh->set(req)) {
			snprintf(buffer, RES_BUFFER_SIZE-1, "\
HTTP/1.%d 203 Error\r\n\
Server: ISL HTTP server %s\r\n\
Connection: Close\r\n\
\r\n\
Cannot set value",
				1, SERVER_VERSION);
		conn->write(buffer, strlen(buffer));
		return 0;
			
		};
	}
	
	conh->create(req, this);
	
	if ((code == 302) && (location != NULL)) {
		/* Moved temporarly */
		snprintf(buffer, RES_BUFFER_SIZE-1, "\
HTTP/1.1 302 Moved Temporarily\r\n\
Server: ISL HTTP server %s\r\n\
Content-Type: text/plain\r\n\
Location: %s\r\n\
Connection: Close\r\n\
\r\n\
",
			SERVER_VERSION, location);
		conn->write(buffer, strlen(buffer));
		return 0;
	}
	
	if (code != 200) {
		if (content == NULL) {
			snprintf(buffer, RES_BUFFER_SIZE-1, "\
HTTP/1.%d %d Error\r\n\
Server: ISL HTTP server %s\r\n\
Connection: Close\r\n\
Content-Type: text/plain\r\n\
\r\n\
ERROR: %d\r\n\
",
				1, code, SERVER_VERSION, code);
			conn->write(buffer, strlen(buffer));
		} else {
			snprintf(buffer, RES_BUFFER_SIZE-1, "\
HTTP/1.%d %d Error\r\n\
Server: ISL HTTP server %s\r\n\
Connection: Close\r\n\
Content-Type: %s\r\n\
Content-Length: %d\r\n\
%s%s%s\
\r\n\
\r\n",
				1, code, SERVER_VERSION, 
				content_type, content_length,
				(content_coding ? "Content-Coding: " : ""),
				(content_coding ? content_coding : ""),
				(content_coding ? "\r\n" : ""));
		}
		conn->write(buffer, strlen(buffer));
		if (content) conn->write(content, content_length);
		return 0;
	}
	snprintf(buffer, RES_BUFFER_SIZE-1, "\
HTTP/1.%d 200 OK\r\n\
Date: %s\r\n\
Expires: %s\r\n\
Server: ISL HTTP server %s\r\n\
Content-Length: %d\r\n\
Content-Type: %s\r\n\
%s%s%s\
\r\n\
",
			1, 
			date,
			exp_date,
			SERVER_VERSION,
			content_length, content_type,
			(content_coding ? "Content-Coding: " : ""),
			(content_coding ? content_coding : ""),
			(content_coding ? "\r\n" : ""));
	sz_log(10, "Will write header");
	conn->write(buffer, strlen(buffer));
	sz_log(10, "Will write content");
	conn->write(content, content_length);
	sz_log(10, "Content written");
	return 0;
}

/**
 * Parsed given URI from client's request.
 * @param uri request URI
 */
ParsedURI::ParsedURI(char *uri) 
{
	int tokc1, tokc2, tokc3, i;
	char **toks1, **toks2, **toks3;
	NodeList *n = NULL;
	OptionList *l = NULL;

	nodes = NULL;
	options = NULL;
	last_slash = ((uri[strlen(uri)-1]) == '/');
	tokc1 = 0;
	tokenize_d(uri, &toks1, &tokc1, "?");
	tokc2 = 0;
	tokenize_d(toks1[0], &toks2, &tokc2, "@");
	tokc3 = 0;
	tokenize_d(toks2[0], &toks3, &tokc3, "/");
	
	for (i = 0; i < tokc3; i++) {
		if (nodes == NULL) {
			n = new NodeList();
			nodes = n;
		} else {
			n->next = new NodeList();
			n = n->next;
		}
		n->next = NULL;
		n->name = strdup(toks3[i]);
	}
	tokenize_d(NULL, &toks3, &tokc3, "");
	
	if (tokc2 > 1)
		attribute = strdup(toks2[1]);
	else
		attribute = NULL;
	if (tokc1 <= 1) {
		tokenize_d(NULL, &toks2, &tokc2, "");
		tokenize_d(NULL, &toks1, &tokc1, "");
		
		return;
	}
	tokenize_d(toks1[1], &toks2, &tokc2, "&");
	for (i = 0; i < tokc2; i++) {
		tokenize_d(toks2[i], &toks3, &tokc3, "=");
		if (tokc3 > 1) {
			if (options == NULL) {
				options = new OptionList();
				l = options;
			} else {
				l->next = new OptionList();
				l = l->next;
			}
			l->name = strdup(toks3[0]);
			l->value = strdup(toks3[1]);
			l->next = NULL;
		}
	}
	tokenize_d(NULL, &toks3, &tokc3, "");
	tokenize_d(NULL, &toks2, &tokc2, "");
	tokenize_d(NULL, &toks1, &tokc1, "");
}

/**
 * Returns value of option from URI. Options in URI are strings of form
 * "option=value", delimited by "&" chars. They appear in URI after "?" char.
 * Example is:
 * "http://localhost:8081/Kociol_1/Waga@masa?output=html&header=none"
 * @param name name of options
 * @return value of option, NULL if option doesn't exist of is empty
 */
char *ParsedURI::getOption(const char *name) 
{
	OptionList *l;
	
	for (l = options; l; l = l->next)
		if (!strcmp(l->name, name))
			return l->value;
	return NULL;
}

ParsedURI::~ParsedURI(void)
{
	NodeList *n, *n1;
	OptionList *o, *o1;

	for (n = nodes; n; n = n1) {
		n1 = n->next;
		free(n->name);
		delete n;
	}
	for (o = options; o; o = o1) {
		o1 = o->next;
		free(o->name);
		free(o->value);
		delete o;
	}
	delete attribute;
}


std::vector<int> Server::StartAll(ConfigLoader *cloader, AbstractContentHandler *conh)
{
	std::vector<int> ret = {};
	
	char * sections = cloader->getString("servers", NULL);
	char **toks = NULL;
	int tokc = 0;
	tokenize(sections, &toks, &tokc);
	if (tokc < 0) {
		throw std::runtime_error("No servers found");
	}

	for (int i = 0; i < tokc; i++) {
		ConnectionFactory *conf = NULL;
		ConnectionHandler *ch = NULL;
		AccessManager *access_manager = NULL;
		Server *server = NULL;
		if (!strcmp(cloader->getStringSect(toks[i], "ssl", "no"), "yes")) {
			sz_log(2, "creating HTTPS server");
			conf = new SSLServer(
					strdup(cloader->getStringSect(toks[i], "cert","server.pem")),
					strdup(cloader->getStringSect(toks[i], "key","server.pem")), 
					NULL, 
					strdup(cloader->getStringSect(toks[i], "keypass","paramd"))
					);
		} else {
			sz_log(2, "creating HTTP server");
			conf = new ConnectionFactory();
		}
		if (!strcmp(cloader->getStringSect(toks[i], "auth", "no"), "yes")) {
			sz_log(2, "Authorization on");
			// initialize access manager
			access_manager = new AccessManager(strdup(
					cloader->getStringSect(toks[i], "access",
						"access.conf")));
			// initialize handler for connections
			ch = new AuthConnectionHandler(conh, 
					cloader->getIntSect(toks[i], "timeout", 30), 
					conf, access_manager);
	
		} else {
			sz_log(2, "Authorization off");
			ch = new ConnectionHandler(conh, conf, 
					cloader->getIntSect(toks[i], "timeout", 30));
		}
		if (strcmp(cloader->getStringSect(toks[i], "allowed_ip", "*"),
					"*")) {
			ch->set_allowed_ip(cloader->getStringSect(toks[i],
						"allowed_ip", NULL));
		}
		server = new Server(
				cloader->getIntSect(toks[i], "port", 8081 + i), 
				ch); 
		conh->configure(cloader, toks[i]);
		const int fork_status = fork();
		switch (fork_status) {
			case 0: // child, start server
				tokenize(NULL, &toks, &tokc);
				delete cloader;
				server->start(); // never returns
				sz_log(0, "http server exiting on error");
				throw std::runtime_error("server exited");
			case -1 : //error
				sz_log(0, "fork() error");
				throw std::runtime_error("fork() failed");
			default : // parent, free memory
				ret.push_back(fork_status);
				delete server;
				delete ch;
				delete access_manager;
				delete conf;
		}
	}
	tokenize(NULL, &toks, &tokc);
	return ret;
}
