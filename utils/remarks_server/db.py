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

from psycopg2.extensions import adapt as psycoadapt
from psycopg2.extensions import register_adapter
from twisted.enterprise import adbapi
import psycopg2

class Database:
	def __init__(self, config):
		db_args = {}
		db_args['database'] = config.get("database", "name")
		db_args['user'] = config.get("database", "user")
		db_args['password'] = config.get("database", "password")

		if config.has_option("database", "host"):
			db_args['host'] = config.get("database", "host")

		self.dbpool = adbapi.ConnectionPool("psycopg2",
			cp_reconnect = True,
			cp_openfun = lambda conn : conn.set_client_encoding('utf-8'),
			**db_args)

		server_name = config.get("database", "server_name")
		connection = psycopg2.connect(**db_args)
		cursor = connection.cursor()

		cursor.execute("SELECT id from server where name = %(name)s", { 'name' : server_name })
		self.server_id = cursor.fetchall()[0][0]

		cursor.close()
		connection.close()
		



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

