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
 * TCP/IP server.
 * $Id$
 */

#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <openssl/ssl.h>
#include <stdint.h>
#include <libxml/parser.h>

#include "uri.h"
#include "config_load.h"

#include <vector>

/** memory cleaning macro */
#define CLEAR(a)	if (a) { \
				xmlFree(a); \
				a = NULL; \
			}

/** maximum allowed length of client's request */
#define HTTP_MAX_REQUEST_LEN 8192

class Connection;
class ConnectionHandler;
class AbstractContentHandler;
class ContentHandler;
class HTTPResponse;
class ParsedURI;
class HTTPRequest;

/**
 * Class responsible for creating new connections. It just calls Connection
 * class constructor, but subclasses can have aditional functionality like
 * initializing some common data.
 */
class ConnectionFactory {
	public:
	ConnectionFactory();
	virtual ~ConnectionFactory() {};
	virtual int init(void);
	virtual Connection *new_connection(ConnectionHandler *ch,
			int socket, uint32_t addr);
	protected:
	int is_init;    /**< "is properly initialized" flag */
};

/**
 * Class responsible for single connection handling. 
 */
class ConnectionHandler {
	public:
		/**
		 * This calls ConnectionHandler(conh, 30).
		 */
		ConnectionHandler(AbstractContentHandler *_conh,
				ConnectionFactory *_conf) : 
			conh(_conh),
			conf(_conf),
			timeout(30),
			allowed_ip(NULL)
		{ };
		virtual ~ConnectionHandler();
		/**
		 * @param _timeout initial value for timeout attribute
		 */
		ConnectionHandler(AbstractContentHandler *_conh,
				ConnectionFactory *_conf,
				int _timeout) : 
			conh(_conh),
			conf(_conf),
			timeout(_timeout),
			allowed_ip(NULL)
		{ };
		virtual int handle(int sock_num, uint32_t addr);
		virtual int do_handle(Connection *conn);
		/**
		 * Set allowed IP.
		 * @param ip_list string with expresion suitable for matching 
		 * with text representation of clients IP (using fnmatch). 
		 * Copy is made, previous content of allowed_ip attribute is 
		 * freed.
		 */
		void set_allowed_ip(char *ip_list);
	protected:
		/**
		 * Check if client with given IP is allowed to connect to
		 * server.
		 * @param addr client's IP address
		 * @return 1 if allowed, 0 if not
		 */
		int is_allowed(uint32_t addr);
		AbstractContentHandler *conh;
				/**< Content handler - responsible for creating
				 * content for response. */
		ConnectionFactory *conf; /**< Connection factory */
		int timeout;	/**< Timeout (in seconds) to cancel inactive 
				  connection. */
		char *allowed_ip;
				/**< fnmatch() expresion for allowed IP
				 * addresses; if NULL clients from all
				 * IP are allowed. */
};

/**
 * Single TCP/IP connection.
 */
class Connection {
	public:
		/**
		 * @param ch "parent" connection handler
		 * @param socket_num socket with active TCP/IP connection
		 */
		Connection(ConnectionHandler *_ch, int socket_num, 
				uint32_t addr);
		virtual ~Connection(void);
		virtual int read(void *buffer, int length);
		virtual int write(void *buffer, int length);
		/**
		 * @return connection's BIO object
		 */
		BIO *get_bio(void)
		{
			return bio;
		}
		/**
		 * @return connection's socket descriptor
		 */
		virtual int get_fd(void)
		{
			return sock_fd;
		}
		ConnectionHandler *ch;
				/**< "Parent" connection handler */
	protected:
		int sock_fd;	/**< Socket file descriptor. */
		BIO *bio;	/**< BIO for socket descriptor. */
	public:
		uint32_t addr;	/**< Client's IP */
};

/**
 * TCP/IP server class.
 */
class Server {
	public:
		/**
		 * @param _port TCP/IP port to listen on
		 * @param _ch connection handling object
		 */
		Server(int _port, ConnectionHandler *_ch) :
			port(_port),
			ch(_ch)
		{ } ;
		virtual ~Server() {};
		virtual int start(void);
		/* Starts servers as described in configuration file loaded by conf object.
		 * For each section new process is created with fork and
		 * server started.
		 * @param conf initialized configuration info object
		 * @param conf object for creating content handler
		 * @returns list of childrens' PIDs
		 */
		static std::vector<int> StartAll(ConfigLoader *conf, AbstractContentHandler *conh);
		
	protected:
		int port;	/**< listen port */
		ConnectionHandler *ch;
				/**< connection handler */
};

/**
 * Simple paser of HTTP client's request. Implements only basic HTTP/1.1 GET 
 * functionality.
 */
class HTTPRequestParser {
	public:
		/**
		 * @param _conn connection to read data from
		 */
		HTTPRequestParser(Connection *_conn) : conn(_conn) 
		{ };
		virtual ~HTTPRequestParser();
		virtual HTTPRequest *parse(int timeout);
	protected:
		Connection *conn;	/**< Actual connection */
};

/**
 * Parsed HTTP client request.
 */
class HTTPRequest {
	public:
		HTTPRequest() :
			suggested_code(200),
			method(NULL),
			uri(NULL),
			auth(NULL),
			protocol_version(0),
			allow_xml(0),
			allow_svg(0),
			unparsed_uri(NULL),
			content_length(0),
			content(NULL)
		{ };
		virtual ~HTTPRequest();
		/**
		 * Parses client request given as string and fills in class 
		 * attributes.
		 * @param str client request
		 */
		virtual void parse(char *string);
		/** Called by parse() method to deal with request content
		 * (if any). May modify content and suggested_code attributes.
		 * @param string request body (including headers)
		 */
		virtual void parse_content(char *string);
		int suggested_code;	
			/**< Suggested HTTP return code, 200 if request is
			 * understood, other specific on error. */
		char *method;
			/**< Method specified in request, currently only
			 * GET, PUT and POST are supported. */
		ParsedURI *uri;
			/**< Parsed uri of demanded resource. */
		char *auth;
			/**< Content of "Authorization" HTTP header, NULL
			 * if doesn't exist. */
		int protocol_version;
			/**< HTTP protocol version: 11 for HTTP/1.1,
			 * 10 for HTTP/1.0 or 0 for other unsupported */
		int allow_xml;
			/**< Does the browser support XML? */
		int allow_svg;
			/**< Does the browser support SVG? */
		char *unparsed_uri;
			/**< Unparsed client's request - for information
			 * purposes only. */
		int content_length;
			/**< Length of request content (for POST or POST) */
		char *content;
			/** Copy of request content. */ 
};

/**
 * Response send for HTTP request.
 */
class HTTPResponse {
	public:
		/**
		 * @param _conn connection 
		 */
		HTTPResponse(Connection *_conn) : 
			code(200),
			date(NULL),
			exp_date(NULL),
			content_type(NULL),
			content_length(0),
			content_coding(NULL),
			location(NULL),
			content(NULL),
			conn(_conn)
		{ };
		virtual ~HTTPResponse();
		virtual int response(HTTPRequest *req, AbstractContentHandler *conh,
				int put = 0);

		int code;	/**< HTTP return code */
		char *date;	/**< Date string */
		char *exp_date;	/**< Expires date string */
		char *content_type;
				/**< HTTP Header Content-Type field */
		int content_length;
				/**< Content length (in octets) */
		char *content_coding;
				/**< Content coding (if any) */
		char *location;	
				/**< Location (used with 302 - Moved
				 Temporarily code) */
		char *content;
				/**< Response content */
	protected:
		Connection *conn;	/**< Actual connection */
};

/**
 * Base abstract class for all clases serving responses content.
 */
class AbstractContentHandler {
	public:
		AbstractContentHandler(void) { };
		virtual ~AbstractContentHandler() { };
		virtual void create(HTTPRequest *req, HTTPResponse *res) = 0;
		virtual int set(HTTPRequest *req) = 0;
		virtual void configure(ConfigLoader *conf, const char* sect) = 0;
};

#endif

