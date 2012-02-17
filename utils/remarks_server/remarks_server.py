#!/usr/bin/python
# -*- encoding: utf-8 -*-

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
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

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

	def new(self, user_id, username):
		"""Create new session, return token for new session.
		"""
		token = self.new_token()
		self.data[token] = (time.time(), "", user_id, username) # (access time, last error string)
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
				(atime, errstr, user_id, username) = self.data[token]
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

class UserParam: 
	def __init__(self):
		self.name = ""
		self.prefix = ""
		self.source = ""
		self.type = ""
		self.unit = ""
		self.start_date = ""
		self.prec = ""
		
	def parse_xml(self, p_string, u_name):

		p_xml = etree.fromstring(p_string)

		p_node = p_xml.xpath("/param")[0]

		self.prefix = p_node.attrib["base"]
		self.name = p_node.attrib["name"]
		self.type = p_node.attrib["type"]
		self.unit = p_node.attrib["unit"]
		self.prec = p_node.attrib["prec"]

		start_date = p_node.attrib["start_date"]
		if not start_date == "-1":
			start_date = start_date
		else:
			start_date = None
		self.start_date = start_date

		self.formula = p_node.text

		
	
class UserSet:
	def __init__(self):
		self.name = None
		self.prefixes = []
		self.draws = []

	def parse_xml(self, u_string, u_name):
		u_xml = etree.fromstring(u_string)

		self.name = u_xml.xpath("/window")[0].attrib["title"]

		for u_draw in u_xml.xpath("/window/param"):

			draw = {}

			draw["name"] = u_draw.attrib["name"]

			prefix = u_draw.attrib["source"]
			draw["source"] = prefix
			self.prefixes.append(prefix)

			draw["hoursum"] = True if "hoursum" in u_draw.attrib else None
			draw["draw"] = u_draw.attrib["draw"]

			draw["max"] = None
			draw["min"] = None
			draw["color"] = None
			draw["title"] = None
			draw["sname"] = None
			draw["scale"] = None
			draw["min_scale"] = None
			draw["max_scale"] = None

			draws = u_draw.xpath("draw")
			if len(draws):
				u_draw = draws[0]
				if "color" in u_draw.attrib:
					draw["color"] = u_draw.attrib["color"]

				if "min" in u_draw.attrib:
					draw["min"] = u_draw.attrib["min"]

				if "max" in u_draw.attrib:
					draw["max"] = u_draw.attrib["max"]

				if "title" in u_draw.attrib:
					draw["title"] = u_draw.attrib["title"]
	
				if "sname" in u_draw.attrib:
					draw["sname"] = u_draw.attrib["sname"]

				if "scale" in u_draw.attrib:
					draw["scale"] = u_draw.attrib["scale"]

				if "min_scale" in u_draw.attrib:
					draw["min_scale"] = u_draw.attrib["scale"]

				if "max_scale" in u_draw.attrib:
					draw["max_scale"] = u_draw.attrib["scale"]


			self.draws.append(draw)



class RemarksXMLRPCServer(xmlrpc.XMLRPC):
	"""Main object communicating with the user"""
	def __init__(self, service):
		xmlrpc.XMLRPC.__init__(self, allowNone=True)

		self.service = service

		self.user_id = None
		self.username = None

	def check_token(self, token):
		ok, self.user_id, self.username = self.service.sessions.check(token)

		if not ok:
			raise xmlrpc.Fault(-1, "Incorrect session token")

	def xmlrpc_login(self, user, password):

		def login_callback(result):
			ok, user_id, username = result
			if not ok:
				raise xmlrpc.Fault(-1, "Incorrect username and password")

			token = self.service.sessions.new(user_id, username)

			return token

		defer = self.service.db.login(user, password)
		defer.addCallback(login_callback)
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
			
			defer = self.service.db.put_remark_into_db(etree.tostring(tree),
						id,
						remark_node.attrib['prefix'])

			defer.addCallback(lambda x: True)

		defer = self.service.db.has_access_to_prefix(prefix, self.user_id)
		defer.addCallback(can_post_remark_callback)
		return defer

	def xmlrpc_get_remarks(self, token, xmlrpc_time, prefixes):

		self.check_token(token)

		defer = self.service.db.get_remarks(xmlrpc_time.value, prefixes, self.user_id)
		defer.addCallback(lambda rows : [ (xmlrpc.Binary(row[0]), row[1], row[2], row[3]) for row in rows] ) 
		return defer

	def xmlrpc_update_params(self, token, params):

		self.check_token(token)

		params_manager = ParamsManager(self.service.db, self.user_id, self.username)

		return params_manager.insert_or_update(params)

	def xmlrpc_remove_params(self, token, params):

		self.check_token(token)

		params_manager = ParamsManager(self.service.db, self.user_id, self.username)

		return params_manager.remove_params(params)

	def xmlrpc_update_sets(self, token, sets):

		self.check_token(token)

		sets_manager = UserSetsManager(self.service.db, self.user_id, self.username)

		return sets_manager.insert_or_update_sets(sets)

	def xmlrpc_remove_sets(self, token, sets):

		self.check_token(token)

		sets_manager = UserSetsManager(self.service.db, self.user_id, self.username)

		return sets_manager.remove_sets(sets)

	def xmlrpc_get_params_and_sets(self, token, xmlrpc_time, prefixes):

		self.check_token(token)

		fetcher = ParamsAndSetsFetcher(self.service.db, self.user_id, xmlrpc_time.value, prefixes)

		return fetcher.do_it()


class ParamsAndSetsFetcher:
	def __init__(self, database, user_id, time, prefixes):
		self.db = database
		self.user_id = user_id
		self.prefixes = prefixes
		self.time = time

	def do_it(self):
		return self.db.dbpool.runInteraction(self._do_it_interaction)

	def _do_it_interaction(self, trans):
		tdb = TransDbAccess(self.db, trans)

		self.get_prefixes(tdb)
		self.get_params(tdb)
		self.get_sets(tdb)

		return (self.params, self.sets)

	def get_prefixes(self, tdb):
		prefixes_ids = tdb.get_prefix_ids_for_names(self.prefixes);
		user_prefixes = tdb.get_user_prefixes(self.user_id)

		self.first_time_fetched_prefixes = user_prefixes - prefixes_ids
		self.already_fetched_prefixes = user_prefixes & prefixes_ids
		
	def get_params(self, tbd):
		params = []

		params.extend(tbd.get_params(self.already_fetched_prefixes, self.time))
		params.extend(tbd.get_params(self.first_time_fetched_prefixes, psycopg2.TimestampFromTicks(0)))

		self.params = params

	def get_sets(self, tdb):

		s1 = tdb.get_draws(self.already_fetched_prefixes, self.time)
		s2 = tdb.get_draws(self.first_time_fetched_prefixes, psycopg2.TimestampFromTicks(0))

		for k, v in s2.iteritems():
			if not k in s1:
				s1[k] = v
			else:
				if v[0] == False:
					continue
				else:
					s1[k][3].extend(v[3])

		set_names = {}

		for set_id in s1.keys():
			set_names[set_id] = tdb.get_set_name(set_id)

		sets = []

		for k, v in s1.iteritems():
			sets.append((set_names[k], v))

		self.sets = sets

class UserSetsManager:
	def __init__(self, database, user_id, username):
		self.db = database
		self.user_id = user_id
		self.username = username

	def insert_or_update_sets(self, sets):
		return self.db.dbpool.runInteraction(self._insert_or_update_sets_interaction, sets)

	def _insert_or_update_set(self, user_set, tdb):

		prefix_map = {}

		for d in user_set.draws:
			prefix = d["source"]
			if not prefix in prefix_map:
				prefix_id = tdb.access_to_prefix(prefix, self.user_id)
				if prefix_id is None:
					return False
				prefix_map[prefix] = prefix_id

			d["prefix_id"] = prefix_map[prefix]

		set_id, set_user_id = tdb.get_set_id(user_set.name)

		if set_id is None:
			set_id = tdb.insert_set(user_set.name, self.user_id)
		elif set_user_id == self.user_id:
			tdb.remove_draws_from_set(set_id)
		else:
			return False

		i = 0
		while i < len(user_set.draws):
			tdb.insert_set_draw(set_id, user_set.draws[i], i)
			i += 1
				
		tdb.update_set_mod_time(set_id, deleted=False)

		return True


	def _insert_or_update_sets_interaction(self, trans, sets):
		tdb = TransDbAccess(self.db, trans)
	
		r = []

		for s in sets:
			user_set = UserSet();
			user_set.parse_xml(str(s), self.username)

			ret = self._insert_or_update_set(user_set, tdb)

			r.append(ret)

		return r
		
	def remove_sets(self, sets):
		return self.db.dbpool.runInteraction(self._remove_sets_interaction, sets)
				
	def _remove_sets_interaction(self, trans, sets):
		tdb = TransDbAccess(self.db, trans)

		r = []

		for s in sets:
			user_set = UserSet();

			user_set.parse_xml(str(s), self.username)

			if not all([tdb.has_access_to_prefix(p, self.user_id) for p in set(user_set.prefixes)]):
				r.append(False)
				continue

			set_id, set_user_id = tdb.get_set_id(user_set.name)
			if self.user_id == set_user_id:
				tdb.update_set_mod_time(set_id, deleted=True)
				r.append(True)
			else:
				r.append(False)

		return r


class ParamsManager:
	def __init__(self, database, user_id, username):
		self.db = database
		self.user_id = user_id
		self.username = username

	def _insert_or_update_interaction(self, trans, params):
		tdb = TransDbAccess(self.db, trans)
		r = []

		for p in params:
			param = UserParam()
			param.parse_xml(str(p), self.username)

			prefix_id = tdb.access_to_prefix(param.prefix, self.user_id)
			if prefix_id is None:
				r.append(False)
				continue

			param_id, param_user_id = tdb.get_param_id(param.name)

			if param_id is None:
				tdb.insert_param(param, self.user_id, prefix_id)
				r.append(True)
			elif param_user_id == self.user_id:
				tdb.update_param(param, param_id, prefix_id, self.user_id)
				r.append(True)
			else:
				r.append(False)

		return r
			
	def insert_or_update(self, params):
		return self.db.dbpool.runInteraction(self._insert_or_update_interaction, params)

	def remove_params(self, params):
		return self.db.dbpool.runInteraction(self._remove_interaction, params)

	def _remove_interaction(self, trans, params):
		r = []
		tdb = TransDbAccess(self.db, trans)
		for p in params:
			param = UserParam()
			param.parse_xml(str(p), self.username)

			if not tdb.has_access_to_prefix(param.prefix, self.user_id):
				r.append(False)
				continue

			r.append(tdb.remove_param(param.prefix, param.name, self.user_id))

		return r

class Database:

	def __init__(self, config):
		db_args = {}
		db_args['database'] = config.get("database", "name")
		db_args['user'] = config.get("database", "user")
		db_args['password'] = config.get("database", "password")
		db_args['cp_openfun'] = lambda conn : conn.set_client_encoding('utf-8')

		if config.has_option("database", "host"):
			db_args['host'] = config.get("database", "host")


		self.dbpool = adbapi.ConnectionPool("psycopg2", cp_reconnect = True , **db_args)

		del db_args['cp_openfun']

		server_name = config.get("database", "server_name")
		connection = psycopg2.connect(**db_args)
		cursor = connection.cursor()

		cursor.execute("SELECT id from server where name = %(name)s", { 'name' : server_name })
		self.server_id = cursor.fetchall()[0][0]

		cursor.close()
		connection.close()
		

	def login(self, user, password):
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

	def get_remarks(self, time, prefixes, id):
	
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

	def has_access_to_prefix_query(self, prefix, user_id):
		return ("""
			SELECT
				prefix_id
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

	def has_access_to_prefix(self, prefix, user_id):
		defer = self.dbpool.runQuery(*self.has_access_to_prefix_query(prefix,user_id))

		defer.addCallback(lambda result: True if len(result[0]) > 0 else None)
		return defer

	def put_remark_into_db(self, remark, prefix): 
		defer = self.dbpool.runOperation("""
			INSERT INTO 
				remark 
					(content, post_time, id, prefix_id, server_id)
				values
					(%(content)s, %(time)s, (select nextval('remarks_seq')), (select id from prefix where prefix = %(prefix)s), %(server_id)s)""",
				{ 'content' : remark,
					'time' : psycopg2.TimestampFromTicks(time.time()),
					'prefix' : prefix,
					'server_id' : self.server_id})

		defer.addCallback(lambda x: True)
		return defer

class TransDbAccess:
	def __init__(self, db, trans):
		self.db = db
		self.trans = trans

	def access_to_prefix(self, prefix, user_id):
		query = self.db.has_access_to_prefix_query(prefix, user_id)
		self.trans.execute(*query)
		r = self.trans.fetchall()
		return r[0][0] if len(r) > 0 else None

	def has_access_to_prefix(self, prefix, user_id):
		return True if self.access_to_prefix(prefix, user_id) is not None else False

	def update_param(self, param, param_id, prefix_id, user_id):
		self.trans.execute("""
			UPDATE
				param
			SET
				prefix_id = %(prefix_id)s, formula = %(formula)s, type=%(type)s, unit=%(unit)s, start_date=%(start_date)s, prec=%(prec)s, mod_time = %(mod_time)s
			WHERE
				id = %(id)s AND user_id=%(user_id)s""",
			{ 'prefix_id' : prefix_id,
				'type' : param.type,
				'unit' : param.unit,
				'start_date' : param.start_date,
				'prec' : param.prec,
				'formula' : param.formula,
				'mod_time' : psycopg2.TimestampFromTicks(time.time()),
				'id' : param_id,
				'user_id' : user_id
				})

	def insert_param(self, param, user_id, prefix_id):
		self.trans.execute(u"""
			INSERT INTO
				param
					(pname, prefix_id, type, unit, formula, start_date, prec, mod_time, user_id)
			VALUES
				(%(pname)s, %(prefix_id)s, %(type)s, %(unit)s, %(formula)s, %(start_date)s, %(prec)s, %(mod_time)s, %(user_id)s)""",
			{
				'pname' : param.name,
				'prefix_id' : prefix_id,
				'type' : param.type,
				'unit' : param.unit,
				'formula' : param.formula,
				'start_date' : param.start_date,
				'prec' : param.prec,
				'mod_time' : psycopg2.TimestampFromTicks(time.time()),
				'user_id' : user_id
				})

	def insert_set(self, name, user_id):
		self.trans.execute("select  nextval('set_seq')");
		set_id = self.trans.fetchall()[0][0]

		self.trans.execute("""
			INSERT INTO
				draw_set (set_id, name, user_id)
			VALUES
				(%(set_id)s, %(name)s, %(user_id)s)
			""", { 'set_id' : set_id, 'name' : name, 'user_id' : user_id})
				
		return set_id

	def remove_draws_from_set(self, set_id):
		self.trans.execute("""
			DELETE FROM
				draw
			WHERE
				set_id = %(set_id)s""",
			{ 'set_id' : set_id })

	def insert_set_draw(self, set_id, draw, draw_order):
		draw["set_id"] = set_id
		draw["draw_order"] = draw_order

		query = u"""
			INSERT INTO
				draw (set_id, name, draw, title, sname, prefix_id, hoursum, color, draw_min, draw_max, scale, min_scale, max_scale, draw_order)
			VALUES
				(%(set_id)s, %(name)s, %(draw)s, %(title)s, %(sname)s, %(prefix_id)s, %(hoursum)s, %(color)s, %(min)s, %(max)s, %(scale)s, %(min_scale)s, %(max_scale)s, %(draw_order)s)"""

		self.trans.execute(query, draw)

		del draw["set_id"]
		del draw["draw_order"]

	def update_set_mod_time(self, set_id, deleted):
		self.trans.execute("""
			UPDATE
				draw_set
			SET
				mod_time = %(time)s, deleted = %(deleted)s
			WHERE
				set_id = %(set_id)s""",
			{ 'set_id' : set_id, 'time' : psycopg2.TimestampFromTicks(time.time()), 'deleted' : deleted } )
				
			
	def get_param_id(self, param):
		self.trans.execute("""
			SELECT
				id, user_id
			FROM	
				param
			WHERE
				pname = %(pname)s
			""", { 'pname' : param })
		r = self.trans.fetchall()
		return r[0][0:2] if len(r) > 0 else (None, None)
	

	def remove_param(self, prefix, name, user_id):
		self.trans.execute("""
			UPDATE	
				param
			SET
				deleted = 't',
				mod_time = %(mod_time)s
			WHERE
				prefix_id = (SELECT id FROM prefix WHERE prefix = %(prefix)s) AND pname = %(pname)s AND user_id = %(user_id)s""",
			{ 'mod_time' : psycopg2.TimestampFromTicks(time.time()), 'prefix' : prefix, 'pname' : name, 'user_id' : user_id})
		return self.trans.rowcount > 0

	def get_prefixes(self):
		self.trans.execute("""
			SELECT
				prefix_id
			FROM
				prefix""")

		ret = []

		row = self.trans.fetchone()
		while row:
			ret.append(int(row[0]))
			row.trans.fetchone()
		
		return ret

	def get_prefix_ids_for_names(self, names):
		self.trans.execute("""
			SELECT
				id
			FROM
				prefix
			WHERE prefix IN %(names)s""",
			{ 'names' : names } )
		
		r = set()
		
		row = self.trans.fetchone()
		while row:
			r.add(row[0])
			row = self.trans.fetchone()

		return r

	def get_user_prefixes(self, user_id):
		self.trans.execute("""	
				SELECT
					prefix_id
				FROM
					user_prefix_access
				WHERE
					user_id = %(id)s
			""", { 'id' : user_id })

		ret = set()

		row = self.trans.fetchone()
		while row:
			ret.add(row[0])
			row = self.trans.fetchone()
		
		return ret

	def get_params(self, prefixes, time):
		self.trans.execute("""	
				SELECT
					p.pname, r.prefix, p.formula, p.type, p.unit, p.start_date, p.prec, p.mod_time, p.deleted 
				FROM
					param as p
				JOIN
					prefix as r
				ON
					(p.prefix_id = r.id)
				WHERE
					p.prefix_id IN %(prefixes)s and p.mod_time >= %(time)s
			""", { 'time' : time, 'prefixes' : prefixes })

		ret = []
		row = self.trans.fetchone()
		while row:
			ret.append(row)
			row = self.trans.fetchone()
		return ret

	def get_set_id(self, set_name):
		self.trans.execute("""
			SELECT
				set_id, user_id
			FROM
				draw_set
			WHERE
				name = %(set_name)s""",
			{ 'set_name' : set_name})

		row = self.trans.fetchone()
		if row:
			return row[0:2]
		else:
			return (None, None)

	def get_set_name(self, set_id):
		self.trans.execute("""
			SELECT
				name
			FROM
				draw_set
			WHERE
				set_id = %(set_id)s""",
			{ 'set_id' : set_id} )

		row = self.trans.fetchone()
		return row[0]


	def get_draws(self, prefixes, time):
		self.trans.execute("""
			SELECT 
				d.set_id, p.prefix, d.name, d.draw, d.title, d.sname, d.hoursum, d.color, d.draw_min, d.draw_max, d.scale, d.min_scale, d.max_scale, ds.deleted, u.name, ds.mod_time
			FROM
				draw as d
			JOIN
				draw_set as ds
			ON
				(d.set_id = ds.set_id)
			JOIN
				prefix as p
			ON
				(d.prefix_id = p.id)
			JOIN
				users as u
			ON
				(ds.user_id = u.id) 
			WHERE
				ds.mod_time >= %(time)s AND d.prefix_id IN %(prefixes)s
			ORDER BY
				d.draw_order""",
			{ 'time' : time, 'prefixes' : prefixes })

		ret = {}

		row = self.trans.fetchone()
		while row:
			set_id = row[0]
			draw = row[1:12]
			deleted = row[13]
			user_name = row[14]
			mod_time = row[15]

			if deleted:
				ret[set_id] = (False,)
			else:
				if set_id in ret:
					ret[set_id][3].append(draw)
				else:
					ret[set_id] = (True, user_name, mod_time, [draw])

			row = self.trans.fetchone()

		return ret


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
register_adapter(set, SQL_IN)

config = ConfigParser.ConfigParser()
config.read(__CONFIG_FILE__)

#application = service.Application('remarks_server', uid=NOBODY_UID, gid=NOGROUP_GID)
application = service.Application('remarks_server')
serviceCollection = service.IServiceCollection(application)

remarks_service = RemarksService(config)
site = server.Site(RemarksXMLRPCServer(remarks_service))

ssl_server = internet.SSLServer(config.getint("connection", "port"), site, SSLContextFactory(config))
ssl_server.setServiceParent(service.IServiceCollection(application))
