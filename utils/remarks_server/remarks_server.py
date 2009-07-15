#!/usr/bin/python

import os
import sys
import time
from lxml import etree
import socket
import logging
import random
import psycopg2
import psycopg2.extensions
from psycopg2.extensions import adapt as psycoadapt
from psycopg2.extensions import register_adapter
import optparse
import xmlrpclib
import SocketServer
import ConfigParser
import BaseHTTPServer
import SimpleHTTPServer
import SimpleXMLRPCServer
from collections import deque
from OpenSSL import SSL

__REMSERVER_CONFIG_FILE__ = "/etc/szarp/remark_server_config.txt"

class SecureXMLRPCServer(BaseHTTPServer.HTTPServer,SimpleXMLRPCServer.SimpleXMLRPCDispatcher):
	def __init__(self, server_address, HandlerClass, options, logRequests=True):
		"""Secure XML-RPC server.
		Similar to SimpleXMLRPCServer but it uses HTTPS for transporting XML data.
		"""
		self.logRequests = logRequests
	
		SimpleXMLRPCServer.SimpleXMLRPCDispatcher.__init__(self, False, 'utf-8')
		SocketServer.BaseServer.__init__(self, server_address, HandlerClass)
		ctx = SSL.Context(SSL.SSLv23_METHOD)
		ctx.use_privatekey_file (options["keyfile"])
		ctx.use_certificate_file(options["certfile"])
		self.socket = SSL.Connection(ctx, socket.socket(self.address_family,
				self.socket_type))
		self.server_bind()
		self.server_activate()

class SecureXMLRpcRequestHandler(SimpleXMLRPCServer.SimpleXMLRPCRequestHandler):
	"""Secure XML-RPC request handler class.
	Based on SimpleXMLRPCRequestHandler but it uses HTTPS for transporting XML data.
	"""
	def setup(self):
		self.connection = self.request
		self.rfile = socket._fileobject(self.request, "rb", self.rbufsize)
		self.wfile = socket._fileobject(self.request, "wb", self.wbufsize)
        
	def do_POST(self):
		"""Handles the HTTPS POST request.
		It was copied out from SimpleXMLRPCServer.py and modified to shutdown the socket cleanly.
		"""
		try:
			# get arguments
			data = self.rfile.read(int(self.headers["content-length"]))
			response = self.server._marshaled_dispatch(
					data, getattr(self, '_dispatch', None)
					)
		except: # This should only happen if the module is buggy
			# internal error, report as HTTP server error
			self.send_response(500)
			self.end_headers()
		else:
			# got a valid XML RPC response
			self.send_response(200)
			self.send_header("Content-type", "text/xml; charset=utf-8")
			self.send_header("Content-length", str(len(response)))
			self.end_headers()
			self.wfile.write(response)
		
			# shut down the connection
			self.wfile.flush()
			self.connection.shutdown() # Modified here!

class SQL_IN(object):
    """Adapt a tuple to an SQL quotable object."""
    
    def __init__(self, seq):
	self._seq = seq

    def prepare(self, conn):
        pass

    def getquoted(self):
        # this is the important line: note how every object in the
        # list is adapted and then how getquoted() is called on it

	qobjs = [str(psycoadapt(o).getquoted()) for o in self._seq]

	if len(qobjs):
		return '(' + ', '.join(qobjs) + ')'
	else:
		#blah
		return '(NULL)'
	
    __str__ = getquoted

register_adapter(tuple, SQL_IN)
register_adapter(list, SQL_IN)

class Database:
	def __init__(self, options):
		self.connection = psycopg2.connect(database=options["name"], user=options["user"], password=options["password"])

	def check_username_password(self, user, password):
		cursor = self.connection.cursor()
		cursor.execute("SELECT real_name, id FROM users WHERE name = %(user)s and password = %(password)s", { 'user' : user, 'password' : password } );

		res = cursor.fetchall()

		ret = (False, (None, None))

		if len(res):
			ret = (True, (res[0][0], res[0][1]))
		cursor.close()

		return ret

	def get_remarks(self, time, prefixes, id):
		cursor = self.connection.cursor()
#		cursor.execute("SELECT content FROM remark WHERE post_time >= %(time)s", { 'time' : time })

		cursor.execute("""
			SELECT 
				content
			FROM
				remark
			WHERE 
				(		
						post_time >= %(time)s
					AND
						prefix_id IN
						(	SELECT
								id
							FROM
								user_prefix_access
							WHERE user_id = %(id)s
						INTERSECT 
							SELECT
								id
							FROM
								prefix
							WHERE
								prefix IN %(prefixes)s
						)
				)
				OR
					prefix_id IN
					(	SELECT
							prefix_id
						FROM
							user_prefix_access
						WHERE user_id = %(id)s
					EXCEPT
						SELECT
							id
						FROM
							prefix
						WHERE
							prefix IN %(prefixes)s
					)
			""", { 'time' : time, 'id' : id, 'prefixes' : prefixes })


		ret = [ xmlrpclib.Binary(row[0]) for row in cursor.fetchall() ]

		cursor.close()
		return ret

	def get_next_remark_id(self):
		cursor = self.connection.cursor()
		cursor.execute("SELECT nextval('remarks_seq')")
		id = cursor.fetchall()[0][0]
		cursor.close()

		return id

	def can_post_remark(self, prefix, user_id):
		cursor = self.connection.cursor()
		cursor.execute("""
			SELECT
				COUNT(*)
			FROM 
				prefix
			JOIN
				user_prefix_access
			ON
				(prefix.id = user_prefix_access.prefix_id)
			WHERE
				prefix.prefix = %(prefix)s AND user_prefix_access.user_id = %(user_id)s

			""",  { 'prefix' : prefix, 'user_id': user_id })

		c = cursor.fetchall()[0][0]	
		cursor = self.connection.cursor()

		if c > 0:
			return True
		else:
			return False

	
	def put_remark_into_db(self, remark, prefix, id, user_id):
		if not self.can_post_remark(prefix, user_id):
			raise xmlrpclib.Fault(-1, "User is not allowed to send remarks for this base")

		cursor = self.connection.cursor()
		cursor.execute("""
			INSERT INTO 
				remark 
					(content, post_time, id, prefix_id)
				values
					(%(content)s, %(time)s, %(id)s, (select id from prefix where prefix = %(prefix)s))""",
				{ 'content' : remark, 'time' : psycopg2.TimestampFromTicks(time.time()), 'id': id, 'prefix' : prefix })
		cursor.close()
		self.connection.commit()

		
	def post_remark(self, remark, author, user_id):
		tree = etree.fromstring(remark.data)

		remark_node = tree.xpath("/remark")[0]
		remark_node.attrib['id'] = str(self.get_next_remark_id())
		remark_node.attrib['author'] = author

		self.put_remark_into_db(etree.tostring(tree), remark_node.attrib['prefix'], remark_node.attrib['id'], user_id)


class Sessions:
	"""Container for all sessions. Stolen from kaboo."""

	def __init__(self):
		"""Init empty container
		"""
		self.data		= {}		 	# dictionary of tokens
		self.otokens		= deque()	 	# deque of id of tokens

		self.expir			= 30 * 60	# session expiration time in seconds
		self.min_token_id	= 1			# Min of token Id
		self.max_token_id	= 100000	# Max of token Id (max - min > 10)

	def new(self):
		"""Create new session, return token for new session.
		"""
		token = self.new_token()
		self.data[token] = (time.time(), "") # (access time, last error string)
		self.otokens.append(token)
		return token

	def check(self, token):
		"""Check is session is active.
		"""
		# check if token is valid
		if not self.data.has_key(token):
			return False
		# check if session not expire
		(atime, errstr) = self.data[token]
		if time.time() - atime > self.expir:
			del self.data[token]
			return False
		# update last access token
		self.data[token] = (time.time(), errstr)
		return True

	def error(self, token):
		"""Return last error message for given session, empty string
		for no-error; cleares error message"
		"""
		(atime, errstr) = self.data[token]
		self.data[token] = (atime, '')
		return errstr

	def set_error(self, token, errstr):
		"""Sets error string for given session.
		"""
		self.data[token] = time.time(), errstr

	def clean_expired_tokens(self):
		"""Purge all expired tokens.
		"""
		try:
			while True:
				token = self.otokens.pop()
				(atime, errstr) = self.data[token]
				if time.time() - atime > self.expir:
					del self.data[token]
				else:
					self.otokens.appendleft(token)
					return;
		except IndexError:
			return


	def new_token(self):
		"""Create new token, private.
		"""
		if len(self.data) > ((self.max_token_id - self.min_token_id) / 4):
			# Cache overflow detected
			# note: div 4 because we draw lots and we don't want do this again and again
			self.max_token_id = 2 * (self.max_token_id + 10)

		token = random.randint(self.min_token_id, self.max_token_id)
		while True:
			# Delete expired tokens
			self.clean_expired_tokens()
			# Check if token is uniq
			if not self.data.has_key(token):
				return token
			# try one more time
			token = random.randint(self.min_token_id, self.max_token_id)


"""
Class guarding access to server resources.
"""
class RemarksServer:
	def __init__(self, db):
		self.db = db
		self.no_token = [ 'login' ]
		self.sessions = Sessions()
		self.users = {}
		
	def login(self, user, password):
		"""Checks users credentials and return new session token on success, False when authorization failed.
		"""
		ok, user = self.db.check_username_password(user, password)
		if ok:
			token = self.sessions.new()
			self.users[token] = user  
			return token
		else:
			raise xmlrpclib.Fault(-1, "Incorrect username and password")

	def post_remark(self, token, remark):
		self.db.post_remark(remark, self.users[token][0], self.users[token][1])
		return True

	def get_remarks(self, token, datetime, prefixes):
		return self.db.get_remarks(datetime.value, prefixes, self.users[token][1])

	def _dispatch(self, method, params):
		"""Special _dispatch method is called for every method call. We use if for checking session token before each method call.
		"""
		if not method in self.no_token:
			# Check for correct session token
			if not self.sessions.check(params[0]):
				raise xmlrpclib.Fault(-1, "Incorrect session token")
		return eval('self.' + method + '(*params)')

class RemarksXMLRPCServer(SecureXMLRPCServer):
	def __init__(self, argv = sys.argv):
		self.args = None
		self.options =  {}
		self.parse_argv(argv)

		if self.args.verbose:
			logging.getLogger('').setLevel(logging.DEBUG)
		else:
			logging.getLogger('').setLevel(logging.ERROR)

		self.parse_config(self.args.config_file)

		SecureXMLRPCServer.__init__(
				self, 
				(self.options["connection"]["host"], int(self.options["connection"]["port"])), 
				SecureXMLRpcRequestHandler, 
				self.options["connection"],
				self.args.verbose
		)

		db = Database(self.options["database"])

		self.register_instance(RemarksServer(db))
		ip, port = self.socket.getsockname()
		logging.info ("Serving HTTPS on %s port %s", ip, port)


	def parse_argv(self, argv):
		"""	Parse commandline arguments and create options object.
		"""
		parser = optparse.OptionParser(
			usage	= "%prog [options]\n\t Program exchange graphs' remarks beetween database and draw3 clients\n\t using HTTPS transport.",
			version	= "%prog 0.1"	
			)
		parser.add_option("-f", "--file", 
			dest="config_file", help="path to config file", default=__REMSERVER_CONFIG_FILE__)
		parser.add_option("-q", "--quiet",
			dest="verbose", action="store_false",
			help="don't print status messages to stdout")
		parser.add_option("-v", "--verbose",
			dest="verbose", action="store_true", default=True)
		self.args = parser.parse_args(argv)[0]

	def parse_config(self, config_file):
		config = ConfigParser.ConfigParser()
		config.read(config_file)

		self.options["database"] = {}
		for i in ["name", "user", "password"]:
			self.options["database"][i] = config.get("database", i)

		self.options["connection"] = {}
		for i in ["certfile", "keyfile", "host", "port"]:
			self.options["connection"][i] = config.get("connection", i)




if __name__ == '__main__':
	try:
		RemarksXMLRPCServer().serve_forever()
	except KeyboardInterrupt:
		pass
	except socket.error, e:
		print "Failed to start to listen for connections (%s)"%(e)

