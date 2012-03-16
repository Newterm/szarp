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
from twisted.web import xmlrpc, server

import transdb

class Remarks:
	def __init__(self, db):
		self.db = db

	def _post_remark_interaction(self, trans, user_id, username, remark):
		tdb = transdb.TransDbAccess(self.db, trans)

		tree = etree.fromstring(remark.data)
		remark_node = tree.xpath("/remark")[0]
		remark_node.attrib['author'] = username
		prefix = remark_node.attrib['prefix']

		prefix_id = tdb.access_to_prefix(prefix, user_id)
		if prefix_id is None:
			raise xmlrpc.Fault(-1, "User is not allowed to send remarks for this base")

		tdb.insert_remark(etree.tostring(tree), prefix_id)

	def post_remark(self, user_id, username, remark):
		return self.db.dbpool.runInteraction(self._post_remark_interaction, user_id, username, remark)

	def _get_remarks_interaction(self, trans, time, prefixes, user_id):
		tdb = transdb.TransDbAccess(self.db, trans)

		prefixes_ids = tdb.get_prefix_ids_for_names(prefixes);
		user_prefixes = tdb.get_user_prefixes(user_id)

		new_prefixes = user_prefixes - prefixes_ids
		known_prefixes = user_prefixes & prefixes_ids

		return tdb.get_remarks(time, known_prefixes, new_prefixes)

	def get_remarks(self, time, prefixes, user_id):
		return self.db.dbpool.runInteraction(self._get_remarks_interaction, time, prefixes, user_id)



