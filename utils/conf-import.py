#!/usr/bin/env python
# -*- coding: ISO-8859-2 -*-
# SZARP: SCADA software 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
# $Id$

"""
 Configuration Manager

 $Id$
"""

try:
	import pysvn
except Exception, e:
	print "This script requires pysvn module, under Debian install python-svn package."
	raise e
from optparse import OptionParser
import shutil
import os
import glob
import tempfile
import time
import re
import tarfile
import sys
import getpass
import gettext
gettext.bindtextdomain('confmgr', '/opt/szarp/resources/locales')
gettext.textdomain('confmgr')
_ = gettext.gettext

class Credentials:
	username = ''
	password = ''
	cache = False
	first = True


def get_login(realm, username, may_save ):
	if Credentials.username =='' or Credentials.first == False:
		print _('Username: '),
		Credentials.username = sys.stdin.readline()[:-1]

	if Credentials.password =='' or Credentials.first == False:
		Credentials.password = getpass.getpass(_('Password:'))

	Credentials.first = False

	return True, Credentials.username, Credentials.password, Credentials.cache


class Options:
	"""Command line option parser"""
	
	ignoredFiles = []
	enabledFiles = []

	def __init__(self):
		self.parseOptions()

	
	def parseOptions(self):
		"""parse command line options and arguments"""

		# parser initialization
		parser = OptionParser(usage=_("""\
usage: %prog [options] [PATH]
Create subdir for SZARP configuration from PATH (defaults to .) in SZARP
configuration SVN repository. PATH can point to specific configuration
or can be main SZARP directory - in this case all subdirectories are
searched for SZARP configuration.
"""))
		parser.add_option('-b','--use-backup', action='store_true',
				dest='useBackup', default=False, 
				help=_('add backup files to repository [default: %default]'))
		
		parser.add_option('-f','--forget-backups', action='store_true',
				dest='forgetBackup', default=False, 
				help=_('treat backup files like normal files [default: %default]'))

		parser.add_option('-i','--ignore-file', action='append',
				dest='ignoredFiles', metavar='PATTERN',
				default= [ '.*\.rap$', 'line.*\.cfg$','^definable.cfg$','^parcook.cfg$','^sender.cfg$', '^PTT\.act$', '^ekrn.*\.cor$', 'szbase_stamp', '.*\.log', 'sysinfo.*' ], 
				help=_('ignore files matching PATTERN; default: %default'))
		
		parser.add_option('-e','--enable-file', action='append',
				dest='enabledFiles', default=[],
				metavar='PATTERN',
				help=_('use ONLY files matching PATTERN [default: %default]'))
		
		parser.add_option('-s','--svn-repository', action='store',
				dest='svnRepo', default=None,
				metavar='ADDRESS',
				help=_('ADDRESS of SVN repository default:%default'))
		parser.add_option('-u','--username', action='store',
				dest='username',default='',
				metavar='NAME',
				help=_('NAME of user accesing repository; -u, -p and -c options do not work with svn+ssh access'))
		
		parser.add_option('-p','--password', action='store',
				dest='password',default='',
				metavar='PASS',
				help=_('PASS of user accesing repository'))

		parser.add_option('-c','--cache-password', action='store_true',
				dest='cache', default=False, 
				help=_('cache passwords [default: %default]'))

		parser.add_option('-a','--ascii-only', action='store_true',
				dest='ascii', default=False, 
				help=_('use only files with ascii names [default: %default]'))
		# parsing
		(opts, args) = parser.parse_args()

		if opts.svnRepo is None:
			parser.error('-s/--svn-repository option is required')

		for i in opts.ignoredFiles:
			self.ignoredFiles.append(re.compile(i))
		
		for i in opts.enabledFiles:
			self.enabledFiles.append(re.compile(i))

		self.useBackup = opts.useBackup
		self.forgetBackup = opts.forgetBackup
		self.svnRepo = opts.svnRepo
		self.ascii = opts.ascii
		Credentials.username = opts.username
		Credentials.password = opts.password
		Credentials.cache = opts.cache
		if len(args)>0:
			self.path = args[0]
		else:
			self.path='.'


class FileAndDirHelper:
	"""File and Directory helper methods."""

	def isConfDir(self,path):
		"""check if given direcory holds configuration"""
		return os.path.exists(path+'/config/params.xml')

	def getConfDirs(self,path):
		"""get configuration subdirs from directory"""
		list = []
		for i in glob.glob(path+'/*/config/params.xml'):
			list.append(i.rsplit('/',2)[0])
		return list
	
	def getFileStamp(self,file):
		"""get time stamp"""

		stamp = time.localtime(os.stat(file)[8])
		return str(stamp[0]).zfill(2)+str(stamp[1]).zfill(2)+str(stamp[2]).zfill(2)+str(stamp[3]).zfill(2)+str(stamp[4]).zfill(2)+str(stamp[5]).zfill(2)

	def getStamp(self,path,cfg):
		"""get stamp from name (ex. config20070101) or
		modyfication date"""

		try:
			if len(cfg)==14:
				int(cfg[6:]) # make sure that this is configYYYYMMDD
				
				c = cfg[6]
				# assertion: 3rd milenium dates only!
				if int(c)==2:
					return cfg[6:]+'000000'
				else: # some other number: date must be reversed
					return cfg[-4:]+cfg[8:10]+cfg[6:8]+'000000'
			elif len(cfg)<14:
				file = path+'/'+cfg
				return self.getFileStamp(file)
			else: # len(cfg) > 14
				if not int(cfg[6:14]) > 0: # make sure that this is configYYYYMMDDsuffix
					raise ValueError # poprably config-YYYYMMDD

				stamp = self.getStamp(path,cfg[:14])
				siblings = self.sortByDates(glob.glob(path+'/'+cfg[:14]+'*'))
				num = 0
				for i in siblings:
					num+=1
					if os.path.split(i[0])[1] == cfg:
						# stamp = normal stamp +
						# sibling number
						return stamp[:-6]+str(num).zfill(6)

		except ValueError:
			if cfg[6]=='-' and len(cfg)==15: # config-YYYYMMDD
				return cfg[7:]+'000000'
			else:
				return self.getFileStamp(path+'/'+cfg)


	def sortByDates(self,files):
		"""sort by dates (time stamps) return sorted list of tulpes
		(file,stamp)"""

		hm = dict()
		for i in files:
			hm[self.getFileStamp(i)]=i
		sorted = hm.keys() # sort stamps
		sorted.sort()
		res = []
		for i in sorted:
			res.append([hm[i],i])
		return res


class Manager:
	"""Configuration manager."""
	
	client = pysvn.Client()
	client.set_auth_cache(Credentials.cache)
	client.callback_get_login = get_login
	options = Options()
	fad = FileAndDirHelper()
	tmp = ''

	def importConfigs(self):
		"""import all data from all configs"""
		path = self.options.path
		self.tmp = tempfile.mkdtemp('','confmgr-');
		if self.fad.isConfDir(path):
			self.importHistory(path) # user gave us conf dir
		else: # configurations are in subdirectorys
			for i in self.fad.getConfDirs(path):
				self.importHistory(i)
	
		os.system('rm -rf '+self.tmp) # FIXME


	def importHistory(self,path):
		"""import history of one configuration"""
		
		prefix = os.path.split(path)[1]
		print _('Importing configuration'), prefix

		historyDirs = dict()

		# for all subdir with historical configurations
		for i in glob.glob(path+'/config*'):
			dir = os.path.split(i)[1] # only subdir name
		
			if os.path.islink(i) != True: # no links
				# get stamp
				stamp = self.fad.getStamp(path,dir)
				historyDirs[stamp]=dir

		self.prepare(prefix) # prepare configuration directory
		self.prepare(prefix+'/config') # prepare config subdir


		# add files from main dir
		self.addFiles(path,prefix,'',True,'')

		sorted = historyDirs.keys()
		sorted.sort() # sort archives (config* dirs)
	
		# add files from first config* dir
		self.addFiles(path+'/'+historyDirs[sorted[0]], prefix, 'config',True, sorted[0])

		# add files from rest config* dirs
		for i in sorted[1:]:
			self.addFiles(path+'/'+historyDirs[i], prefix, 'config',False,i)


	def prepare(self,prefix):
		"""prepare directory for working with svn"""

		tmp_path = self.tmp+'/'+prefix
		repo_path = self.options.svnRepo+'/'+prefix
		tokens = self.options.svnRepo.rstrip("/").split("/")
		if tokens[len(tokens) - 1] == prefix:
			raise Exception("Repo path '%s' last token '%s' mustn't be the same as prefix '%s'" % (self.options.svnRepo, prefix, prefix))
		print "Importing directory '%s' as '%s', continue? [Y/n]" % (os.path.abspath(prefix), repo_path)
		choice = raw_input().lower()
		if not choice in ["", "y"]:
			print "Cancelled by user"
			exit(1)
		try:
			# try to checkout
			self.client.checkout(repo_path,tmp_path)
		except pysvn.ClientError, e:
			print e
			print "Directory doesn't exist, creating.."
			# if failed make directory and import it to
			# repository
			os.mkdir(tmp_path)
			self.client.import_(tmp_path, repo_path, \
					'Creating configuration '+prefix)

		# final checkout - to make sure .svn exist
		self.client.checkout(repo_path,tmp_path)


	def isIgnored(self,file):
	 	"""check if file is in any ignored list"""

		for i in self.options.ignoredFiles:
			if i.match(file) != None:
				return True
		return False

	def isEnabled(self,file):
		"""check if file is in all enabled lists"""
		if self.options.ascii:
			try:
				file.encode('ascii')
			except:
				print 'Rejecting non-ascii |'+ file+'|'
				return False

		for i in self.options.enabledFiles:
			if i.match(file) == None:
				return False
		return True

	def addFiles(self,source, prefix, dir, first, stamp):
		"""adding files (and thier history) to repository"""

		# umpacking tar archives
		tar_tmp = ''
		if os.path.isfile(source):
			if tarfile.is_tarfile(source):
				src_flag = False
				tar = tarfile.open(source)
				# tmp dir - target of extraction
				tar_tmp = tempfile.mkdtemp('','confmgr-tar-');
				source = tar_tmp
				try:
					# files in tar file are in config
					# subdir
					tar.getmember('config/')
				except:
					src_flag = True
					# no config subdir in tar file -
					# creating one
					source += '/config'
					os.mkdir(source)

				# extract all files to tmp dir
				for i in tar.getmembers():
					tar.extract(i,source)
				tar.close()
				if src_flag == False:
					source += '/config'


		# cutting stamp
		if len(stamp) > 14:
			stamp = stamp[:14]
		fileLists = []
		files = glob.glob(source+'/*')
		files.sort()
		backups = []
		backupsDict = dict()
		
		# for all files
		for i in files:
			file = os.path.split(i)[1]

			# checking this is the right file
			# must:
			# - be a file
			# - not be a link
			# - not match any ignored pattern
			# - match any enabled pattern
			# - not be a tar file starting with 'config' string
			if os.path.isfile(i) and not os.path.islink(i) \
					and not self.isIgnored(file) and\
					self.isEnabled(file) and i \
					not in backups and\
					not (tarfile.is_tarfile(i) and\
					file[:6]=='config'):

				# backup files - i-file is thier prefix
				backs = glob.glob(i+'*')

				# backuping
				if not self.options.forgetBackup and len(backs)>1:
					backups += backs
					backupsDict[i] = backs

				# normal route
				if len(backs) == 1 or not (first and self.options.useBackup):
					fileLists.append(i)

		out = self.tmp+'/'+prefix + '/'+dir
	
		# clear out dir
		for i in glob.glob(out+"/*"):
			if os.path.isfile(i):
				os.remove(i)
		
		# copy all good files to out
		for i in fileLists:
			file = os.path.split(i)[1]
			if stamp == '':
				stamp = self.fad.getFileStamp(i)

			if dir != '':
				file = '/'+file

			shutil.copy(i,out)
		
		if stamp =='':
			stamp = '-1'

		# svn status - what have changed 
		changes = self.client.status(out)
	
		# directory is new - we're not adding anything two times
		if self.isNew(out,stamp):
			# add backup files - if user wants to
			if first and self.options.useBackup:
				self.makeBackups(backups,out,backupsDict)

			for f in changes:
				if not os.path.isdir(f.path): # ignoring subdirs
					if f.text_status == pysvn.wc_status_kind.missing:
						# schedule to remove file
						self.client.remove(f.path)
					elif f.text_status == pysvn.wc_status_kind.unversioned:
						# schedule to add file
						self.client.add(f.path)
			# add/remove/modify files
			self.client.checkin(out, 'Modified date: '+stamp)

		if tar_tmp != '':
			os.system('rm -rf '+tar_tmp) # FIXME

	def isNew(self,path,stamp):
		"""checks if this version of file has been already added to
		repository"""

		stamp_i = int(stamp)
		pr = len('Modified date: ')
		# for all log messages of given file
		for i in self.client.log(path):
			try:
				# get stamp from log message
				if stamp_i <= int(i.message[pr:pr+14]):
					return False
			except:
				pass

		return True
	def makeBackups(self,backups,out,backupsDict):
		"""add backup files to repository"""

		backups = self.fad.sortByDates(backups)
		for i in backups: 
			# i[0] backup file
			target = ''
			
			# for all files that have been backuped
			for j in backupsDict.keys():
				# file (i[0]) if one of j's backups
				if i[0] in backupsDict[j]:
					target = out+'/'+os.path.split(j)[1]
					# copy it to tmp dir
					shutil.copy(i[0],target)
					break

			try:
				# try to add to repo
				self.client.add(target)
			except:
				# file already in repository
				pass

			# commit changes
			self.client.checkin(out, 'Modified date: '+i[1])



if __name__ == '__main__':
	mgr = Manager()
	mgr.importConfigs()
