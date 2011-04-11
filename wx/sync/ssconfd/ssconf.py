#!/usr/bin/python

#  SZARP: SCADA software 
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#

"""
ssconfd is a SZARP Synchronization Server's Configuration Daemon.
It's a standalone XML-RPC server, that can read and modify SSS (Szarp
Synchronization Server) users/servers database XML file. Normally is serves
as backend for SSS web administration interface (found in szarp-sssweb 
package).

Users database location is read from 'userdbfile' parameter from section 'sss'
in /etc/szarp/szarp.cfg file (default is /etc/szarp/sss_users.xml).
Configuration for server is in /etc/szarp/ssconf.cfg file and should look
like:

[access]
# Administrator user and password (change it!!!)
admin = admin
password = secret 
[server]
# Name of main server
main = prat
# Address and port to listen on
address = localhost
port = 5500

This file must be readable only by user that ssconfd is runining as, because
it contains administrator password. Every request to server must be authorized
- using user name and password md5 hash of administrator or common SSS user 
(for some operations).

Changes in configuration file are signalled to SSS daemon by sending it -1 
(SIGHUP) signal.
"""

from subprocess import Popen, PIPE
from lxml import etree
import os
import sys
import re
import time
import tempfile
import shutil
import string
import socket
import inspect
from SimpleXMLRPCServer import SimpleXMLRPCServer
import xmlrpclib
import hashlib
from decorator import decorator
import ConfigParser
from random import SystemRandom
import datetime
from optparse import OptionParser
import logging

ss_namespace = "http://www.praterm.com.pl/SZARP/sync-users"
szarp_dir = "/opt/szarp"
szarp_reserved_dirs = ("bin", "resources", "logs", "lib")
empty_base = """<?xml version="1.0" encoding="utf-8"?>
<users xmlns="http://www.praterm.com.pl/SZARP/sync-users">
</users>"""

class AuthException(BaseException):
	"""
	Exception thrown on incorrect user/password
	"""
	pass

def escape_xpath(str):
	"""
	Return copy of str with single quotation marks escaped
	"""
	return str.translate(string.maketrans('',''), "'")

class SSConf:
	"""
	Class for managing SZARP synchronizer users database.
	This class is registered at XML-RPC server.
	"""
	def __init__(self):
		"""
		Constructor
		"""
		self.read_conf()

	def read_conf(self):
		"""
		Read configuration from szarp.cfg file and admin/password from /etc/szarp/ssconf.cfg file.
		"""
		output = Popen(["/opt/szarp/bin/lpparse", "-s", "sss", "userdbfile"], 
				stdout=PIPE).communicate()[0].strip()
		if output == '':
			raise RuntimeError("cannot find 'userdbfile' parameter in 'sss' section of szarp.cfg")

		self.db_path = output
		self.last_stat = 0
		self.db = None

		config = ConfigParser.SafeConfigParser()
		try:
			config.read('/etc/szarp/ssconf.cfg')
			self.admin = config.get('access', 'admin')
			hash = config.get('access', 'password')
			self.main_server = config.get('server', 'main')
			self.address = config.get('server', 'address')
			self.port = config.getint('server', 'port')
		except Exception, e:
			print "Error parsing config file /etc/szarp/ssconf.cfg"
			raise e
		m = hashlib.md5()
		m.update(hash)
		self.passhash = m.hexdigest()

	@decorator
	def checkauth(f, *args, **kw):
		try:
			if args[0].admin != args[1] or args[0].passhash != args[2]:
				raise AuthException("incorrect administrator user/password")
		except KeyError:
			raise AuthException("no user/password supplied")
		return f(*args, **kw)

	@decorator
	def debugex(f, *args, **kw):
		try:
			return f(*args, **kw)
		except Exception, e:
			print e
			print inspect.trace()
			raise e

	@decorator
	def checkuser(f, *args, **kw):
		"""
		Checks for user credentials, set self.user_dict to user data.
		"""
		args[0].read_db()
		u = escape_xpath(args[1])
		p = args[2]
		search = args[0].db.find(".//{%s}user[@name='%s']" % (ss_namespace, u))
		if search is None:
			raise AuthException("invalid user or password")
		if p != search.attrib['password']:
			raise AuthException("invalid user or password")
		args[0].user_dict = dict()
		for k in ('name', 'email', 'expired', 'hwkey', 'server'):
			args[0].user_dict[k] = search.attrib[k]
		bases = args[0].get_available_bases(args[0].admin, args[0].passhash)
		args[0].user_dict['sync'] = filter(lambda x: re.match(search.attrib['sync'], x), bases)
		return f(*args, **kw)
	
	def check_admin(self, user, passhash):
		# check for administrator creditentials
		ret = (self.admin == user) and (self.passhash == passhash)
		if not ret:
			time.sleep(1)
		return ret

	def check_user(self, user, passhash):
		# check for user creditentials
		user = escape_xpath(user)
		self.read_db()
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, user))
		if search is None:
			time.sleep(1)
			return False
		if passhash != search.attrib['password']:
			time.sleep(1)
			return False
		return True

	@debugex
	def read_db(self):
		"""
		Parses XML users db file
		"""
		try:
			st = os.stat(self.db_path).st_mtime
		except IOError, e:
			self.db = etree.XML(empty_base)

		if self.last_stat < st:
			self.db = etree.parse(self.db_path)
			self.last_stat = st

	def write_db(self):
		"""
		Saves changes in user db
		"""
		(tmpfd, tmpname) = tempfile.mkstemp(prefix="sss_userdb")
		try:
			tmpfile = os.fdopen(tmpfd, "wb")
			self.db.write(tmpfile)
			tmpfile.close()
			os.chmod(tmpname, 0644)
			shutil.move(tmpname, self.db_path)
			self.last_stat = os.stat(self.db_path).st_mtime
			os.system("killall -1 sss")
		finally:
			try:
				os.unlink(tmpname)
			except OSError:
				pass

	@checkauth
	def list_users(self, user, passhash):
		"""
		Returns user list
		"""
		self.read_db()
		# find all <user> elements, return list of dictionary attributes
		list = map(lambda x: dict((
			('name', x.attrib['name']), 
			('email', x.attrib['email']),
			('expired', x.attrib['expired']),
			('server', x.attrib['server']),
			('hwkey', x.attrib['hwkey']),
			('comment', x.get('comment','')),
			)), self.db.findall(".//{%s}user" % ss_namespace))
		list.sort(lambda x, y: cmp(x['name'], y['name']))
		return list

	@checkauth
	def list_servers(self, user, passhash):
		"""
		Returns servers list
		"""
		self.read_db()
		list = map(lambda x: dict((
			('name', x.attrib['name']), 
			('ip', x.attrib['ip']),
			('bases', self.get_bases(user, passhash, x.attrib['name'])),
			('main', x.attrib['name'] == self.main_server),
			)), self.db.findall(".//{%s}server" % ss_namespace))
		list.sort(lambda x, y: cmp(x['name'], y['name']))
		return list
	
	@checkauth
	def get_server(self, user, passhash, server_name):
		"""
		Returns servers list
		"""
		self.read_db()
		server_name = escape_xpath(server_name)
		search = self.db.find(".//{%s}server[@name='%s']" % (ss_namespace, server_name))
		if search is None:
			raise RuntimeError('User with name \'%s\' does not exist in database' % server_name)
		ret = dict()
		for k in ('name', 'ip' ):
			ret[k] = search.attrib[k]
		ret['main'] = (ret['name'] == self.main_server)
		ret['bases'] = self.get_bases(user, passhash, server_name)
		return ret

	@checkauth
	def set_server(self, user, passhash, server_dict):
		"""
		Save server info; server_dict contains server attributes. If main is not set, ignore it
		(do not change value)
		"""
		self.read_db()
		server_dict['name'] = escape_xpath(server_dict['name'])
		search = self.db.find(".//{%s}server[@name='%s']" % (ss_namespace, server_dict['name']))
		if search is None:
			raise RuntimeError('Server with name \'%s\' does not exist in database' % server_dict['name'])
		try:
			ip = server_dict['ip']
			socket.getaddrinfo(ip, 80)
		except:
			raise RuntimeException('server address %s is incorrect' % ip)
		search.attrib['ip'] = ip
		try:
			if server_dict['main']:
				search.attrib['main'] = 'yes'
			else:
				del search.attrib['main']
		except KeyError, e:
			pass
		self.write_db();
		return True

	@checkauth
	def add_server(self, user, passhash, server_dict):
		"""
		Add new server
		"""
		self.read_db()
		server_dict['name'] = escape_xpath(server_dict['name'])
		search = self.db.find(".//{%s}server[@name='%s']" % (ss_namespace, server_dict['name']))
		if search is not None:
			raise RuntimeError('Server with name \'%s\' already in database' % server_dict['name'])
		element = etree.Element("{%s}server" % ss_namespace)
		try:
			ip = server_dict['ip']
			socket.getaddrinfo(ip, 80)
		except:
			raise RuntimeException('server address %s is incorrect' % ip)
		element.attrib['name'] = server_dict['name']
		element.attrib['ip'] = server_dict['ip']
		self.db.getroot().append(element)
		self.write_db();
		return True

	@checkauth
	def remove_server(self, user, passhash, name):
		self.read_db()
		name = escape_xpath(name)
		search = self.db.find(".//{%s}server[@name='%s']" % (ss_namespace, name))
		if (search is None) or (len(search) > 0):
			raise RuntimeError('server with name \'%s\' not in database' % name)
		self.db.getroot().remove(search)
		self.write_db()
		return True

	@checkauth
	def add_user(self, user, passhash, user_dict):
		"""
		Add user, user_dict is dictionary like the one obtained from list_users
		Return new user password.
		"""
		self.read_db()
		user_dict['name'] = escape_xpath(user_dict['name'])
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, user_dict['name']))
		if search is not None:
			raise RuntimeError('user with name \'%s\' already in database' % user_dict['name'])
		element = etree.Element("{%s}user" % ss_namespace)
		for (key, val) in user_dict.items():
			if key == 'sync':
				val = '|'.join(val)
			element.attrib[key] = val
		m = hashlib.md5()
		password = self.new_password(user, passhash)
		m.update(password)
		element.attrib['password'] = m.hexdigest()
		element.attrib['basedir'] = szarp_dir
		self.db.getroot().insert(0, element)
		self.write_db();
		return password

	@checkauth
	def set_user(self, user, passhash, user_dict):
		"""
		TODO - input verification
		Set user attributes from user_dict
		"""
		self.read_db()
		user_dict['name'] = escape_xpath(user_dict['name'])
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, user_dict['name']))
		if (search is None) or (len(search) > 0):
			raise RuntimeError('user with name \'%s\' not in database' % user_dict['name'])
		for (key, val) in user_dict.items():
			if key == 'sync':
				val = '|'.join(val)
			if key in ('comment',) and val == '':
				continue
			search.attrib[key] = val
		self.write_db();
		return True

	@checkauth
	def remove_user(self, user, passhash, name):
		"""
		Remove user with name 'name'.
		"""
		self.read_db()
		name = escape_xpath(name)
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, name))
		if (search is None) or (len(search) > 0):
			raise RuntimeError('user with name \'%s\' not in database' % name)
		self.db.getroot().remove(search)
		self.write_db()
		return True

	@debugex
	@checkauth
	def get_user(self, user, passhash, name):
		"""
		Get user info.
		"""
		self.read_db()
		name = escape_xpath(name)
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, name))
		if search is None:
			raise RuntimeError('User with name \'%s\' does not exist in database' % name)
		ret = dict()
		for k in ('name', 'email', 'expired', 'hwkey', 'server'):
			ret[k] = search.attrib[k]
		for k in ('comment',):
			ret[k] = search.get(k, '')
		bases = self.get_available_bases(user, passhash)
		ret['sync'] = filter(lambda x: re.match(search.attrib['sync'], x), bases)
		return ret

	@checkauth
	def get_available_bases(self, user, passhash):
		"""
		Return list of all available szarp bases
		"""
		ret = sorted(filter(lambda x: (x is not None) and (x not in szarp_reserved_dirs) and x.isalnum(), 
			os.listdir(szarp_dir)))
		return ret

	@checkauth
	def get_bases(self, user, passhash, server):
		"""
		Return list of bases for server
		"""
		return self.get_available_bases(user, passhash)

	@checkauth
	def disable_key(self, user, passhash, name):
		"""
		Disable hardware key for user with name 'name'
		"""
		self.read_db()
		name = escape_xpath(name)
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, name))
		if (search is None) or (len(search) > 0):
			raise RuntimeError('user with name \'%s\' not in database' % name)
		search.attrib['hwkey'] = '0'
		self.write_db()
		return True

	@checkauth
	def turnoff_key(self, user, passhash, name):
		"""
		Turn off key checking for given user
		"""
		self.read_db()
		name = escape_xpath(name)
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, name))
		if (search is None) or (len(search) > 0):
			raise RuntimeError('user with name \'%s\' not in database' % name)
		search.attrib['hwkey'] = ''
		self.write_db()
		return True

	@checkauth
	def reset_key(self, user, passhash, name):
		"""
		Reset hardware key for user with name 'user_name'
		"""
		self.read_db()
		name = escape_xpath(name)
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, name))
		if (search is None) or (len(search) > 0):
			raise RuntimeError('user with name \'%s\' not in database' % name)
		search.attrib['hwkey'] = '-1'
		self.write_db()
		return True

	@checkauth
	def set_key(self, user, passhash, name, key):
		"""
		Set waiting key for user 'name' to 'key'
		"""
		if len(key) <= 2:
			raise RuntimeError('set_key: key to short for user \'%s\'' % name)
		self.read_db()
		name = escape_xpath(name)
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, name))
		if (search is None) or (len(search) > 0):
			raise RuntimeError('user with name \'%s\' not in database' % name)
		if search.attrib['hwkey'] != '-1':
			raise RuntimeError('cannot set key for user \'%s\' - key is different from \'-1\'' % name)
		search.attrib['hwkey'] = key
		self.write_db()
		return True

	@checkauth
	def new_password(self, user, passhash):
		"""
		Return new random password
		"""
		chars = '23456qwertasdfgzxcvbQWERTASDFGZXCVB789yuiophjknmYUIPHJKLNM'
		r = SystemRandom()
		return ''.join(r.sample(chars, 8))

	@checkauth
	def reset_password(self, user, passhash, user_name):
		"""
		Generate new user password for user and return it so it can be send back to user.
		"""
		password = self.new_password(user, passhash)
		m = hashlib.md5()
		m.update(password)
		self.set_password(user, passhash, user_name, m.hexdigest())
		return password

	@checkauth
	def set_password(self, user, passhash, name, newpasshash):
		self.read_db()
		name = escape_xpath(name)
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, name))
		if search is None:
			raise RuntimeError('user with name \'%s\' not in database' % name)
		search.attrib['password'] = newpasshash
		self.write_db()

	@checkuser
	def user_get_info(self, user, passhash):
		"""
		Return info for specified user
		"""
		return self.user_dict

	def get_password_link(self, user):
		"""
		Generate hashed key for current date, user and password. This key is used in
		activation link to reset forgotten password. Because hash contains date, 
		user and password, it can be used to reset only current password and only within
		one day.
		Return (key, email) tupple or (False, False) if user is not found.
		"""
		self.read_db()
		user = escape_xpath(user)
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, user))
		if search is None:
			return (False, False)
		m = hashlib.md5()
		m.update("USER%sPASSWORD%sDATE%s" % (user, search.attrib['password'], str(datetime.date.today())))
		return (m.hexdigest(), search.attrib['email'])

	def activate_password_link(self, user, key):
		"""
		Check if activation key is valid for user. It yes, return (new password, e-mail)
		tupple, otherwise (False, False).
		"""
		self.read_db()
		user = escape_xpath(user)
		search = self.db.find(".//{%s}user[@name='%s']" % (ss_namespace, user))
		if search is None:
			return (False, False)
		m = hashlib.md5()
		m.update("USER%sPASSWORD%sDATE%s" % (user, search.attrib['password'], str(datetime.date.today())))
		if key != m.hexdigest():
			return (False, False)
		return (self.reset_password(self.admin, self.passhash, user), search.attrib['email'])

	@checkuser
	def user_change_password(self, user, passhash, newpasshash):
		"""
		User sets new password
		"""
		self.set_password(self.admin, self.passhash, user, newpasshash) 
		return True
	
	def test(self):
		users =  self.list_users(self.user, self.passhash)
		print users
		print self.list_servers(self.user, self.passhash)
		print self.get_user(self.user, self.passhash, users[0]['name'])
		print self.get_bases(self.user, self.passhash, self.list_servers(self.magic)[0]['name'])
		self.add_user(self.user, self.passhash, users[0])

def test():
	s = SSConf()
	s.test()

if __name__ == "__main__":
	usage = """
Server mode:\t\tssconf.py [ -n ]
Change-key mode:\tssconf.py { -c | --changekey } KEY { -u | --user } USER
Usage info:\t\tssconf.py { -h | --help }"""
	parser = OptionParser(usage=usage)
	parser.add_option("-n", "--no-fork", action="store_false", dest="fork", default=True,
				                    help="do not go into background")
	parser.add_option("-c", "--changekey", dest="key", 
				help="change key to KEY, you must also use --user",
				metavar="KEY")
	parser.add_option("-u", "--user", dest="user", 
				help="user name for --changekey option",
				metavar="USER")

	(options, args) = parser.parse_args()

	# Change-key mode
	if options.key or options.user:
		if not (options.key and options.user):
			parser.error("options -c and -u must be used together")
		ssc = SSConf() # the easiest way to read config ;-)
		server = xmlrpclib.ServerProxy("http://%s:%d/" % (ssc.address, ssc.port))
		server.set_key(ssc.admin, ssc.passhash, options.user, options.key)
	else:
		# Server mode
		logging.basicConfig(filename="/opt/szarp/logs/ssconf.log", level=logging.INFO)
		# daemonize
		if options.fork:
			pid = os.fork()
			if pid != 0:
				# parent process, kill myself
				sys.exit()

		logging.info("Starting ssconf")
		ssc = SSConf()
		server = SimpleXMLRPCServer((ssc.address, ssc.port), logRequests = (not options.fork))
		server.register_instance(ssc)
		try:
			server.serve_forever()
		except Exception(e):
			logging.error("Unexpected exception: " + str(e))
			raise e

