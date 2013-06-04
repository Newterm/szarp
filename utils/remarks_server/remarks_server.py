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
#PREFIX_SYNC_INTERVAL = 3600.0
PREFIX_SYNC_INTERVAL = 5.0
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

        # Calculate md5 hash of configuration file for given szbase prefix name
        def get_prefix_config_md5(self, prefix):
                try:
                        filestring = open("/opt/szarp/" + prefix + "/config/params.xml", "r").read()
                except IOError:
                        return ""
                
                md5 = hashlib.md5()
                md5.update(filestring)
                return md5.hexdigest()

        # Looping method for updating configuration file hash
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

        # Database and szbase synchronization
        def do_prefix_sync(self):
                # Compare prefixes in database with /opt/szarp directory perform synchronization
                # Check config/aggr.xml and modify aggregated map
                prefix_map = {}

                for prefix_dir in os.walk(SZARP_DIR).next()[1]:
                        if prefix_dir not in NOT_A_PREFIX_LIST:
                                # check if aggregated
                                prefix_map[prefix_dir] = []
                                try:
                                        treeAggr = ET.parse(SZARP_DIR + "/" + prefix_dir + "/config/aggr.xml")
                                        treeAggrRoot = treeAggr.getroot()
                                        treeAggrConfigs = treeAggrRoot.findall("{http://www.praterm.com.pl/IPK/aggregate}config")
                                        for treeAggrConfig in treeAggrConfigs:
                                                prefix_map[prefix_dir].append(treeAggrConfig.attrib['prefix'])

                                except IOError:
                                        print "Not aggregated"

                self.sync_update_interaction(prefix_map)

        # Update database prefixes and aggregated map
        def _sync_update_interaction(self, trans, prefix_map):
                tdb = transdb.TransDbAccess(self.service.db, trans)
                tdb.sync_update(prefix_map)

        def sync_update_interaction(self, prefix_map):
		return self.service.db.dbpool.runInteraction(self._sync_update_interaction, prefix_map)

        # Database users and cfglogin szbases sync
        #def do_cfglogin_sync(self):
                # Check all prefixes for cfglogin file and change/create cfglogin users accordingly
        
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

# Also start looping task for updating configuration file hash
remarks_server = RemarksXMLRPCServer(remarks_service)

# Also fetch autologin usernames (szbase prefixes) from database
remarks_server.fetch_cfglogin_interaction()

sync_task = task.LoopingCall(remarks_server.do_prefix_sync)
sync_task.start(PREFIX_SYNC_INTERVAL)

hash_update_task = task.LoopingCall(remarks_server.hash_update)
hash_update_task.start(HASH_UPDATE_INTERVAL)

site = server.Site(remarks_server)

ssl_server = internet.SSLServer(config.getint("connection", "port"), site, SSLContextFactory(config))
ssl_server.setServiceParent(service.IServiceCollection(application))
