#!/usr/bin/python

#  SZARP: SCADA software 

#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.

#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USAjj

from twisted.application import internet, service
from twisted.internet import protocol, reactor, defer
from twisted.protocols import basic
from twisted.enterprise import adbapi
from twisted.web import xmlrpc, server
from twisted.python import log
from psycopg2.extensions import adapt as psycoadapt
from psycopg2.extensions import register_adapter
import psycopg2
from OpenSSL import SSL
from collections import deque
from lxml import etree
import ConfigParser
import random
import time
import calendar

__CONFIG_FILE__ = "/etc/szarp/remark_server_config.ini"

NOBODY_UID = 65534
NOGROUP_GID = 65534

class SSLContextFactory:
	"""Factory creating ssl contexts"""
	def __init__(self, config):
		self.certfile = config.get("connection", "certfile")
		self.keyfile = config.get("connection", "keyfile")

	def getContext(self):
		ctx = SSL.Context(SSL.SSLv23_METHOD)
		ctx.use_certificate_file(self.certfile)
		ctx.use_privatekey_file(self.keyfile)
		return ctx

class Sessions:
	"""Container for all sessions. Stolen from kaboo."""

	def __init__(self):
		"""Init empty container
		"""
		self.data		= {}		 	# dictionary of tokens
		self.otokens		= deque()	 	# deque of id of tokens

		self.expir		= 30 * 60	# session expiration time in seconds
		self.min_token_id	= 1			# Min of token Id
		self.max_token_id	= 100000	# Max of token Id (max - min > 10)

	def new(self, user_id, user_name):
		"""Create new session, return token for new session.
		"""
		token = self.new_token()
		self.data[token] = (time.time(), "", user_id, user_name) # (access time, last error string)
		self.otokens.append(token)
		return token

	def check(self, token):
		"""Check if session is active.
		"""
		# check if token is valid
		if not self.data.has_key(token):
			return (False, None, None)
		# check if session not expire
		(atime, errstr, user_id, username) = self.data[token]
		if time.time() - atime > self.expir:
			del self.data[token]
			return (False, None, None)
		# update last access token
		self.data[token] = (time.time(), errstr, user_id, username)
		return (True, user_id, username)

	def clean_expired_tokens(self):
		"""Purge all expired tokens.
		"""
		tokens = deque()
		for token in self.otokens:
			if self.data.has_key(token):
				(atime, errstr, user_id, user_name) = self.data[token]
				if time.time() - atime > self.expir:
					del self.data[token]
				else:
					tokens.append(token)
		self.otokens = tokens


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


class RemarksXMLRPCServer(xmlrpc.XMLRPC):
	"""Main object communicating with the user"""
	def __init__(self, service):
		xmlrpc.XMLRPC.__init__(self)

		self.service = service

		self.user_id = None
		self.username = None

	def check_token(self, token):
		ok, self.user_id, self.username = self.service.sessions.check(token)

		if not ok:
			raise xmlrpc.Fault(-1, "Incorrect session token")

	def xmlrpc_login(self, user, password):

		def login_query_callback(result):
			ok, user_id, username = result
			if not ok:
				raise xmlrpc.Fault(-1, "Incorrect username and password")

			token = self.service.sessions.new(user_id, username)

			return token

		defer = self.service.db.login_query(user, password)
		defer.addCallback(login_query_callback)
		return defer

	def xmlrpc_post_remark(self, token, remark):
		self.check_token(token)

		tree = etree.fromstring(remark.data)

		remark_node = tree.xpath("/remark")[0]
		remark_node.attrib['author'] = self.username
		prefix = remark_node.attrib['prefix']

		def can_post_remark_callback(ok):
			if not ok:
				raise xmlrpc.Fault(-1, "User is not allowed to send remarks for this base")
			
			defer = self.service.db.get_next_remark_id_query()
			defer.addCallback(get_next_remark_id_callback)
			return defer

		def get_next_remark_id_callback(id):
			defer = self.service.db.put_remark_into_db_query(etree.tostring(tree),
						id,
						remark_node.attrib['prefix'])

			defer.addCallback(lambda x: True)
			return defer

		defer = self.service.db.can_post_remark_query(prefix, self.user_id)
		defer.addCallback(can_post_remark_callback)
		return defer

	def xmlrpc_get_remarks(self, token, xmlrpc_time, prefixes):

		self.check_token(token)

		defer = self.service.db.get_remarks_query(xmlrpc_time.value, prefixes, self.user_id)
		defer.addCallback(lambda rows : [ (xmlrpc.Binary(row[0]), row[1], row[2], row[3]) for row in rows] ) 
		return defer


class Database:

	def __init__(self, config):
		db_args = {}
		db_args['database'] = config.get("database", "name")
		db_args['user'] = config.get("database", "user")
		db_args['password'] = config.get("database", "password")

		if config.has_option("database", "host"):
			db_args['host'] = config.get("database", "host")


		self.dbpool = adbapi.ConnectionPool("psycopg2", **db_args)

		server_name = config.get("database", "server_name")
		connection = psycopg2.connect(**db_args)
		cursor = connection.cursor()

		cursor.execute("SELECT id from server where name = %(name)s", { 'name' : server_name })
		self.server_id = cursor.fetchall()[0][0]

		cursor.close()
		connection.close()
		

	def login_query(self, user, password):
		def callback(result):
			if len(result):
				return True, result[0][0], result[0][1]
			else:
				return False, None, None
	
		defer = self.dbpool.runQuery("""
				SELECT
					id, real_name
				FROM
					users
				WHERE
					name = %(user)s and password = %(password)s
				""",
				{ 'user' : user, 'password' : password } );

		defer.addCallback(callback)
		return defer
	
	def get_remarks_query(self, time, prefixes, id):
	
		defer = self.dbpool.runQuery("""
			SELECT 
				remark.content, remark.server_id, prefix.prefix, remark.id
			FROM
				remark 
			JOIN
				prefix
			ON	
				(remark.prefix_id = prefix.id)
			WHERE 
					(		
						post_time >= %(t)s
					AND
						prefix_id IN
						(
							SELECT
								prefix_id
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
					(	
						SELECT
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
					)""",
			{ 't' : time, 'id' : id, 'prefixes' : prefixes })

		return defer

	def can_post_remark_query(self, prefix, user_id):
		defer = self.dbpool.runQuery("""
			SELECT
				COUNT(*)
			FROM 
				prefix
			JOIN
				user_prefix_access
			ON
				(prefix.id = user_prefix_access.prefix_id)
			WHERE
					prefix.prefix = %(prefix)s
				AND
					user_prefix_access.user_id = %(user_id)s
				AND
					user_prefix_access.write_access = 1

			""",  { 'prefix' : prefix, 'user_id': user_id })

		defer.addCallback(lambda result: True if result[0][0] > 0 else False)
		return defer

	def put_remark_into_db_query(self, remark, id, prefix): 
		defer = self.dbpool.runOperation("""
			INSERT INTO 
				remark 
					(content, post_time, id, prefix_id, server_id)
				values
					(%(content)s, %(time)s, %(id)s, (select id from prefix where prefix = %(prefix)s), %(server_id)s)""",
				{ 'content' : remark,
					'time' : psycopg2.TimestampFromTicks(time.time()),
					'id': id,
					'prefix' : prefix,
					'server_id' : self.server_id})

		defer.addCallback(lambda x: True)
		return defer

	def get_next_remark_id_query(self):
		defer = self.dbpool.runQuery("SELECT nextval('remarks_seq')")
		defer.addCallback(lambda result: str(result[0][0]))
		return defer

class RemarksService(service.Service):

	def __init__(self, config):
		self.sessions = Sessions()
		self.db = Database(config)
	
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

config = ConfigParser.ConfigParser()
config.read(__CONFIG_FILE__)

application = service.Application('remarks_server', uid=NOBODY_UID, gid=NOGROUP_GID)
serviceCollection = service.IServiceCollection(application)

remarks_service = RemarksService(config)
site = server.Site(RemarksXMLRPCServer(remarks_service))

ssl_server = internet.SSLServer(config.getint("connection", "port"), site, SSLContextFactory(config))
ssl_server.setServiceParent(service.IServiceCollection(application))
