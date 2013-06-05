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

import hashlib

import psycopg2
import time
from twisted.web import xmlrpc

from twisted.python import log

import datetime

class TransDbAccess:
	def __init__(self, db, trans):
		self.db = db
		self.trans = trans

        def sync_update(self, prefix_map):

		self.trans.execute("""
                        SELECT 
                                prefix.prefix, 
                                aggregators.aggregated  
                        FROM 
                                prefix 
                        LEFT JOIN 
                                (SELECT 
                                        aggregated_map.aggregated AS aggregator, 
                                        prefix.prefix 
                                AS 
                                        aggregated 
                                FROM 
                                        prefix 
                                INNER JOIN 
                                        aggregated_map 
                                ON
                                        prefix.id = aggregated_map.prefix) 
                                AS aggregators 
                                
                        ON prefix.id = aggregators.aggregator;
			""")

                rows = self.trans.fetchall()
                db_map = {}

                if rows:
                        for row in rows:
                                if row[0] not in db_map:
                                        db_map[row[0]] = []
                                if row[1]:
                                        db_map[row[0]].append(row[1])
                

                # Prefixes in /opt/szarp, not in database
                diff = list(set(prefix_map.keys()) - set(db_map.keys()))
                for missing in diff:
                        self.trans.execute("""
                                INSERT INTO
                                        prefix
                                VALUES
                                        (DEFAULT, %(prefix)s)
			                """,
			        { 'prefix' : missing })
                        
                        db_map[missing] = []

                # Modifications of aggregated_map
                for key in db_map.keys():
                        if key in prefix_map.keys():
                                aggr_diff = list(set(prefix_map[key]) - set(db_map[key]))
                                if aggr_diff:
                                        for prefix in aggr_diff:
                                                self.trans.execute("""
                                                        INSERT INTO
                                                                aggregated_map
                                                        VALUES
                                                                ((SELECT id FROM prefix WHERE prefix = %(prefix)s), 
                                                                 (SELECT id FROM prefix WHERE prefix = %(aggregated)s))
			                                """,
                                                        { 'prefix' : prefix, 'aggregated' : key })
                
        def hash_update(self, prefix, config_hash):

                #log.msg("Debug: hash_update("+prefix+","+config_hash+")")

		self.trans.execute("""
			SELECT
				id, password
			FROM
				users
			WHERE
				name = %(user)s and autologin = 1
			""",
			{ 'user' : prefix } )
	
		row = self.trans.fetchone()
		if row:
                        if config_hash != row[1]:

                                log.msg("Debug: hash_update("+prefix+","+row[1]+" -> "+config_hash+")")

		                self.trans.execute("""
                                        UPDATE 
                                                users
	                                SET 
                                                password = %(password)s 
                                        WHERE 
                                                users.id = %(id)s
			                """,
			        { 'password' : config_hash, 'id' : row[0] } )

        def fetch_cfglogin(self):
                # Fetch users using cfglogin
                self.trans.execute("""
                        SELECT
                                name
                        FROM
                                users
                        WHERE
                                autologin = 1
                        """)

                rows = self.trans.fetchall()

                # Convert query result to list of prefixes
                cfglogin_list = []
                if rows:
                        for row in rows:
                                row_list = list(row)
                                cfglogin_list = cfglogin_list + row_list

                return cfglogin_list

	def autologin(self, user, password):

                # autologin format: login@prefix
                user_data = user.split('@')

                if len(user_data) != 2:
                        return False, None, None

                log.msg("Debug: autologin detected: " + user)
        
                # Login a prefix-wide type user
                ok, user_id, username = self.login(user_data[1], password)

		if ok:
                        log.msg("Debug: User entry (" + user_data[1] + ", " + password + ") is authorised (config files in sync)")
                        # Configuration files are in sync
			return ok, user_id, user_data[0]
                else:
                        log.msg("Debug: User entry (" + user_data[1] + ", " + password + ") is not authorised (checking if username exists)")
                        # Check if user login exists in users at all
                        self.trans.execute("""
                                SELECT
                                        id
                                FROM
                                        users
                                WHERE
                                        name = %(name)s
                                """,
                                { 'name' : user_data[1] } )

                        row = self.trans.fetchone()

                        if row is None:
                                log.msg("Debug: User entry (" + user_data[1] + ", " + password + ") incorrect username")
                                return False, None, None
                        
                        user_id = row[0]

                        # Check hash history for given user_id
                        log.msg("Debug: User entry (" + user_data[1] + ", " + password + ") username ok (checking hash history)")
                        self.trans.execute("""
			        SELECT
				        date_created
			        FROM
				        hash_history
			        WHERE
				        user_id = %(user_id)s and password = %(password)s
			        """,
                                { 'user_id' : user_id, 'password' : password } )
	
                        rows = self.trans.fetchall()
                        
                        # We have a list of hash history for given user_id.
                        # It is a list because passwords in history are not
                        # unique. We need to see if the newest date is not 
                        # to old for autologin.

                        #for row in rows:
                        #        log.msg("hash_hist row: " + str(row[0]))
                        if rows:
                                date_list = []
                                for row in rows:
                                        row_list = list(row)
                                        date_list = date_list + row_list

                                print date_list
                                date_list.sort(reverse=True)
                                print date_list
                                # Check if not too old
                                # If newest timestamp is older than now() by more than 93 days do not autologin
                                if datetime.timedelta(days=93) < datetime.datetime.now() - date_list[0]:
                                        log.msg("Debug: User entry (" + user_data[1] + ", " + password + ") password too old")
                                        return False, None, None
                                else:
                                        log.msg("Debug: User entry (" + user_data[1] + ", " + password + ") password found in hash history")
                                        return True, user_id, user_data[0]
                        else:
                                log.msg("Debug: User entry (" + user_data[1] + ", " + password + ") password incorrect")
                                return False, None, None

	def login(self, user, password):
		self.trans.execute("""
			SELECT
				id, real_name
			FROM
				users
			WHERE
				name = %(user)s and password = %(password)s
			""",
			{ 'user' : user, 'password' : password } )
	
		row = self.trans.fetchone()
		if row is not None:
			return True, row[0], row[1]
		else:
			return False, None, None

	def access_to_prefix(self, prefix, user_id):
		self.trans.execute("""
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
		self.trans.execute("select  nextval('set_seq')")
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

        # @hary: sname > short
	def insert_set_draw(self, set_id, draw, draw_order):
		draw["set_id"] = set_id
		draw["draw_order"] = draw_order

		query = u"""
			INSERT INTO
				draw (set_id, name, draw, title, short, prefix_id, hoursum, color, draw_min, draw_max, scale, min_scale, max_scale, draw_order)
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


        # @hary: d.sname > d.short
	def get_draws(self, prefixes, time):
		self.trans.execute("""
			SELECT 
				d.set_id, p.prefix, d.name, d.draw, d.title, d.short, d.hoursum, d.color, d.draw_min, d.draw_max, d.scale, d.min_scale, d.max_scale, ds.deleted, u.name, ds.mod_time
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

	def get_remarks(self, time, known_prefixes, new_prefixes):
		self.trans.execute("""
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
						prefix_id IN %(known_prefixes)s

					)
				OR 
					prefix_id IN %(new_prefixes)s
			""", { 't' : time, 'known_prefixes' : known_prefixes, 'new_prefixes' : new_prefixes})

		ret = []
		row = self.trans.fetchone()
		while row:
			ret.append((xmlrpc.Binary(row[0]), row[1], row[2], row[3]))
			row = self.trans.fetchone()
		return ret

	def insert_remark(self, remark, prefix_id):
		self.trans.execute("""
			INSERT INTO 
				remark 
					(content, post_time, id, prefix_id, server_id)
				values
					(%(content)s, %(time)s, (select nextval('remarks_seq')), %(prefix_id)s, %(server_id)s)
			""", { 'content' : remark, 'time' : psycopg2.TimestampFromTicks(time.time()), 'prefix_id' : prefix_id, 'server_id' : self.db.server_id})

