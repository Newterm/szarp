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
from twisted.internet import protocol
from twisted.internet import task
from twisted.protocols import basic
from twisted.web import xmlrpc, server
from twisted.python import log
from OpenSSL import SSL

import hashlib
import os

import ConfigParser

import sys
sys.path.append("/opt/szarp/lib/python")

import dbinteraction
import sessions
import db
import transdb
import remarks
import paramssets

import xml.etree.ElementTree as ET

__CONFIG_FILE__ = "/etc/szarp/remark_server_config.ini"

NOBODY_UID = 65534
NOGROUP_GID = 65534
HASH_UPDATE_INTERVAL = 10.0
NOT_A_PREFIX_LIST = ["bin","lib","logs","resources"]
SZARP_DIR = "/opt/szarp"

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


class RemarksXMLRPCServer(xmlrpc.XMLRPC):
	"""Main object communicating with the user"""
	def __init__(self, service):
		xmlrpc.XMLRPC.__init__(self, allowNone=True)
		self.service = service
                self.cfglogin_prefixes = []
                self.db_map = {}

	def check_token(f):
		def check(self, token, *args, **kwargs):
			ok, userid, username = self.service.sessions.check(token)
			if ok == True:
				return f(self, userid, username, *args, **kwargs)
			else:
				raise xmlrpc.Fault(-1, "Incorrect session token")
                return check
        
        def _login_interaction(self, trans, user, password):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                ok, user_id, username = tdb.login(user, password)
                if ok:
                        token = self.service.sessions.new(user_id, username)
		        return token
	        else:
                        # Login with configuration hash
                        ok, user_id, username = tdb.autologin(user, password)
                        if ok:
                                token = self.service.sessions.new(user_id, username)
                                return token
                        else:
                                raise xmlrpc.Fault(-1, "Incorrect username and password")

	def xmlrpc_login(self, user, password):
		return self.service.db.dbpool.runInteraction(self._login_interaction, user, password)

	@check_token
	def xmlrpc_post_remark(self, user_id, username, remark):
		return remarks.Remarks(self.service.db).post_remark(user_id, username, remark)

	@check_token
	def xmlrpc_get_remarks(self, user_id, username, xmlrpc_time, prefixes):
		return remarks.Remarks(self.service.db).get_remarks(xmlrpc_time.value, prefixes, user_id)

	@check_token
	def xmlrpc_update_params(self, user_id, username, params):
		params_manager = paramssets.ParamsManager(self.service.db, user_id, username)
		return params_manager.insert_or_update(params)

	@check_token
	def xmlrpc_remove_params(self, user_id, username, params):
		params_manager = paramssets.ParamsManager(self.service.db, user_id, username)
		return params_manager.remove_params(params)

	@check_token
	def xmlrpc_update_sets(self, user_id, username, sets):
		sets_manager = paramssets.UserSetsManager(self.service.db, user_id, username)
		return sets_manager.insert_or_update_sets(sets)

	@check_token
	def xmlrpc_remove_sets(self, user_id, username, sets):
		sets_manager = paramssets.UserSetsManager(self.service.db, user_id, username)
		return sets_manager.remove_sets(sets)

	@check_token
	def xmlrpc_get_params_and_sets(self, user_id, username, xmlrpc_time, prefixes):
		fetcher = paramssets.ParamsAndSetsFetcher(self.service.db, user_id, xmlrpc_time.value, prefixes)
		return fetcher.do_it()

        # This Xml-Rpc method is meant to be called by user to sync /opt/szarp with remarks database
        # after /opt/szarp has changed.
        def xmlrpc_prefix_sync(self):
                prefix_map = {}

                # Check /opt/szarp contents
                for prefix_dir in os.walk(SZARP_DIR).next()[1]:
                        if prefix_dir not in NOT_A_PREFIX_LIST:
                                # check if szbase prefix is aggregated
                                prefix_map[prefix_dir] = []
                                filepath = SZARP_DIR + "/" + prefix_dir + "/config/aggr.xml"
                                if os.path.isfile(filepath):
                                        try:
                                                treeAggr = ET.parse(filepath)
                                                treeAggrRoot = treeAggr.getroot()
                                                treeAggrConfigs = treeAggrRoot.findall("{http://www.praterm.com.pl/IPK/aggregate}config")
                                                for treeAggrConfig in treeAggrConfigs:
                                                        prefix_map[prefix_dir].append(treeAggrConfig.attrib['prefix'])
                                        except IOError:
                                                log.msg("aggr.xml expected but not found")

                # Update database using /opt/szarp prefix map

                # Get database map +
                self.db_map_interaction()
                # Update prefixes in database (add new prefixes to db_map) +
                self.prefix_update_interaction(prefix_map, self.db_map)
                # Update aggregated information +
                self.aggregated_update_interaction(prefix_map, self.db_map)
                # Create new autologin users for added prefixes 
                diff = list(set(prefix_map.keys()) - set(self.db_map.keys()))
                for username in diff:
                        configuration_hash = remarks_server.get_prefix_config_md5(username)
                        if configuration_hash != "": 
                                # Add user +
                                self.add_user_interaction(username, configuration_hash)
                                # Add user_prefix_access +
                                self.add_user_prefix_access_interaction(username)
                                if prefix_map[username]:
                                        # Add aggregated information for username aggregator +
                                        self.add_user_prefix_access_aggregated_interaction(username)
                                        # Update write permissions +
                                        self.update_user_prefix_access_aggregated_interaction(username)
                                # Update cfglogin list for hash update purposes +
                                self.fetch_cfglogin_interaction()

        # Get map of database 
        def _db_map_interaction(self, trans):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                db_map = tdb.db_map()
        def db_map_interaction(self):
		return self.service.db.dbpool.runInteraction(self._db_map_interaction)

        # Update prefixes
        def _prefix_update_interaction(self, trans, prefix_map, db_map):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                tdb.prefix_update(prefix_map, db_map)
        def prefix_update_interaction(self, prefix_map, db_map):
		return self.service.db.dbpool.runInteraction(self._prefix_update_interaction, prefix_map, db_map)
        
        # Update aggregated
        def _aggregated_update_interaction(self, trans, prefix_map, db_map):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                tdb.aggregated_update(prefix_map, db_map)
        def aggregated_update_interaction(self, prefix_map, db_map):
		return self.service.db.dbpool.runInteraction(self._aggregated_update_interaction, prefix_map, db_map)

        # Add user
        def _add_user_interaction(self, trans, username, password):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                tdb.add_user(username, password)
        def add_user_interaction(self, username, password):
		return self.service.db.dbpool.runInteraction(self._add_user_interaction, username, password)
       
        # Add user prefix access
        def _add_user_prefix_access_interaction(self, trans, username):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                tdb.add_user_prefix_access(username)
        def add_user_prefix_access_interaction(self, username):
		return self.service.db.dbpool.runInteraction(self._add_user_prefix_access_interaction, username)
            
        # Add user prefix access aggregated
        def _add_user_prefix_access_aggregated_interaction(self, trans, username):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                tdb.add_user_prefix_access_aggregated(username)
        def add_user_prefix_access_aggregated_interaction(self, username):
		return self.service.db.dbpool.runInteraction(self._add_user_prefix_access_aggregated_interaction, username)

        # Update user prefix access aggregated
        def _update_user_prefix_access_aggregated_interaction(self, trans, username):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                tdb.update_user_prefix_access_aggregated(username)
        def update_user_prefix_access_aggregated_interaction(self, username):
		return self.service.db.dbpool.runInteraction(self._update_user_prefix_access_aggregated_interaction, username)

        # Calculate md5 hash of configuration file for given szbase prefix name
        def get_prefix_config_md5(self, prefix):
                try:
                        filestring = open("/opt/szarp/" + prefix + "/config/params.xml", "r").read()
                except IOError:
                        return ""
                
                md5 = hashlib.md5()
                md5.update(filestring)
                return md5.hexdigest()

        # Updating configuration file hash
        def hash_update(self):
                for prefix in self.cfglogin_prefixes:
                        config_hash = self.get_prefix_config_md5(prefix)
                        if config_hash:
                                self.hash_update_interaction(prefix, config_hash)
        
        def _hash_update_interaction(self, trans, prefix, config_hash):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                tdb.hash_update(prefix, config_hash)

        def hash_update_interaction(self, prefix, config_hash):
		return self.service.db.dbpool.runInteraction(self._hash_update_interaction, prefix, config_hash)

        # Fetching cfglogin prefix list
        def _fetch_cfglogin_interaction(self, trans):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                self.cfglogin_prefixes = tdb.fetch_cfglogin()

        def fetch_cfglogin_interaction(self):
		return self.service.db.dbpool.runInteraction(self._fetch_cfglogin_interaction)
        
        # Add user to database
        def add_user_interaction(self, prefix, password_hash):
		return self.service.db.dbpool.runInteraction(self._add_user_interaction, prefix, password_hash)

        def _add_user_interaction(self, trans,  prefix, password_hash):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                tdb.add_user(prefix, password_hash)

        
class RemarksService(service.Service):
	def __init__(self, config):
		self.sessions = sessions.Sessions()
		self.db = db.Database(config)
	
config = ConfigParser.ConfigParser()
config.read(__CONFIG_FILE__)

#application = service.Application('remarks_server', uid=NOBODY_UID, gid=NOGROUP_GID)
application = service.Application('remarks_server')
serviceCollection = service.IServiceCollection(application)

remarks_service = RemarksService(config)

remarks_server = RemarksXMLRPCServer(remarks_service)

'''
# First: fix database:
# Standalone script adds:
# - Users autologin column,
# - Hash history table,
# - Hash update function,
# - Hash update trigger procedure.
# Remarks server does:
# - Check if exists/add autologin user for each prefix in szarp root dir
for prefix_dir in os.walk(SZARP_DIR).next()[1]:
        if prefix_dir not in NOT_A_PREFIX_LIST:
                remarks_server.add_user_interaction(prefix_dir, remarks_server.get_prefix_config_md5(prefix_dir))
'''

# Initial autologin users from database, updated after each prefix sync
remarks_server.fetch_cfglogin_interaction()

# TASK: Update password hash of users using cfglogin
hash_update_task = task.LoopingCall(remarks_server.hash_update)
hash_update_task.start(HASH_UPDATE_INTERVAL)

site = server.Site(remarks_server)

ssl_server = internet.SSLServer(config.getint("connection", "port"), site, SSLContextFactory(config))
ssl_server.setServiceParent(service.IServiceCollection(application))
