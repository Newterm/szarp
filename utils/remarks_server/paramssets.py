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

from lxml import etree
import psycopg2
import transdb

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

			draw_element_attribs = ["max", "min", "color", "title", "sname", "scale", "min_scale", "max_scale"]
			for attrib in draw_element_attribs:
				draw[attrib] = None

			draws = u_draw.xpath("draw")
			if len(draws):
				u_draw = draws[0]
				for attrib in draw_element_attribs:
					if attrib in u_draw.attrib:
						draw[attrib] = u_draw.attrib[attrib]
			self.draws.append(draw)

class ParamsAndSetsFetcher:
	def __init__(self, database, user_id, time, prefixes):
		self.db = database
		self.user_id = user_id
		self.prefixes = prefixes
		self.time = time

	def do_it(self):
		return self.db.dbpool.runInteraction(self._do_it_interaction)

	def _do_it_interaction(self, trans):
		tdb = transdb.TransDbAccess(self.db, trans)

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
		tdb = transdb.TransDbAccess(self.db, trans)
	
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
		tdb = transdb.TransDbAccess(self.db, trans)

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
		tdb = transdb.TransDbAccess(self.db, trans)
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
		tdb = transdb.TransDbAccess(self.db, trans)
		for p in params:
			param = UserParam()
			param.parse_xml(str(p), self.username)

			if not tdb.has_access_to_prefix(param.prefix, self.user_id):
				r.append(False)
				continue

			r.append(tdb.remove_param(param.prefix, param.name, self.user_id))

		return r

