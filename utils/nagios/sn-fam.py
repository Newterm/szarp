#!/usr/bin/python
"""
  SZARP SCADA
  $Id$
  2009 Pawel Palucha <pawel@praterm.pl>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA


  Daemon for notifying nagios3 monitoring service about changes in SZARP
  configurations and database directories.
  It uses FAM to monitor a list of SZARP directories. When change is detected,
  passive notifications is written to nagios external commands file.
  Config file (default /etc/szarp/szarp-nagios.cfg) is used to list /opt/szarp
  subdirectories that should be excluded from monitoring, shell wildcards can
  be used; for example:

  [Main]
  ignored = pre1 pre2 som? sm*
"""

SZARPLIB = "/opt/szarp/lib"

import sys
# common szarp-configuration related functions
sys.path.append(SZARPLIB)
import szarpnagios as sn

import errno
import _fam
import logging
import logging.handlers
import os
import pty
import select
import signal
import time
import traceback
from optparse import OptionParser
import ConfigParser
from xml.sax import SAXParseException

class SzarpMonitor:
	class LADRequest:
		"""
		Requests container, members:
		@member fr fam monitor for szbase directory
		@member frp fam monitor for params.xml file
		@member tests list of test parameters data for configuration
		"""
		def __init__(self,):
			self.fr = None		# directory monitor
			self.frp = None		# params.xml monitor
			self.tests = []		# list of test parameters
			self.last = None	# timestamp of last check
			self.failed = 0		# check failed, do not perform next checks if service
						# entered HARD state (failed >= 4)
			self.max_delay = 20	# maximum delay in minutes (4 checks to change state to hard)

	def __init__(self, output, timeout, conffile):
		"""
		@param output file to print commands to
		@param timeout number of seconds after event to notify nagios
		"""
		self.changed = set()
		self.toreload = set()
		self.reqs = {}
		self.fam = None
		self.timeout = timeout
		self.first = False
		self.output = output
		self.conffile = conffile

	def processEvent(self, fe, now):
		"""
		Process event, add prefix to self.changed and to self.toreload if params.xml was changed
		@param fe fam event
		@param now current time - to update timestamp of last check
		"""
		if fe.filename[0] == '.':
			# rsync creates lots of temporary files with names starting with '.'
			return
		code = fe.code2str()
		if code[0] == 'c': # 'created' or 'changed'
			self.changed.add(fe.userData)
			if fe.filename[0] == '/':
				# full path - so params.xml was modified
				self.toreload.add(fe.userData)

	def print_lad(self, outfile, prefix, parameter_path, service_name):
	        """
	        Print nagios external command for last-available-date for prefix and parameter
		@param output file to print to
	        @param prefix SZARP configuration prefix
	        @param parameter_path directory for parameter (for example: Status/Meaner3/program_uruchomiony)
	        @service_name name of service to print
	        @return status code, 0 for ok
	        """
	        d = sn.get_lad(prefix, parameter_path)
	        if d is None:
	                code = 3
	                output = "Status parameter not found"
	        else:
	                output = d.strftime("%Y-%m-%d %H:%M")
	                age = sn.get_age(d)
	                if age > self.reqs[prefix].max_delay:
	                        code = 2
	                elif age > self.reqs[prefix].max_delay - 10:
	                        code = 1
	                else:
	                        code = 0
	        print >> outfile, ("[%d] PROCESS_SERVICE_CHECK_RESULT;%s;%s;%d;%s" % (
	                        int(time.time()), prefix, service_name, code, output
	                        ))
		if code == 2:
			self.reqs[prefix].failed += 1 
		else:
			self.reqs[prefix].failed = 0
	        return code
		
	def print_test(self, outfile, prefix, test):
	        """
	        Print external commands for device test
		@param output file to print to
	        @param prefix SZARP configuration prefix
	        @param test (device num, device daemon, parameter) tuple
	        """
	        (num, daemon, param) = test
	        service = "Device %d" % num
		code = False
		if param is None:
	                code = 3
	                output = "Test parameter not defined"
			d = None
		if not code:
			path = sn.param_to_path(param)
	        	d = sn.get_lad(prefix, path)
		if not code and d is None:
	                code = 3
			output = "Data not found: " + path
		if not code:
	                output = daemon + " " + path.replace("/", ":", 2)
	                age = sn.get_age(d)
	                if age > 40:
	                        code = 2
	                elif age > 20:
	                        code = 1
	                else:
	                        code = 0
	        print >> outfile, ("[%d] PROCESS_SERVICE_CHECK_RESULT;%s;%s;%d;%s" % (
	                        int(time.time()), prefix, service, code, output
	                        ))
		return code

	def notifyNagios(self, now, all = False):
		"""
		Send notifications to nagios
		@param now current timestamp
		@param all if True, send notifications about all parameters
		"""
		if all:
			to_send = self.reqs.keys()
		else:
			to_send = self.changed
		count = 0
		for prefix in to_send:
			self.reqs[prefix].last = now
			count += 1
			self.print_lad(self.output, prefix, "Status/Meaner3/program_uruchomiony", "Current data available")
			for test in self.reqs[prefix].tests:
				count += 1
				self.print_test(self.output, prefix, test)
		self.changed.clear()
		self.output.flush()
		if count > 0:
			logging.debug("sent %d notifications", count)

	def initRequests(self):
		"""
		Initialize all fam requests
		"""
		config = ConfigParser.SafeConfigParser()
		config.read(self.conffile)
		ignored = config.get('Main', 'ignored').split()
		logging.debug("list of ignored configurations: %s", " ".join(ignored))
		if self.fam is not None:
			self.closeRequests()
		self.fam = _fam.open()
		prefixes = sn.get_prefixes(ignored)
		logging.debug("list of prefixes found: %s", " ".join(prefixes))
		for prefix in prefixes:
			dir = sn.SZARPDIR + "/%s/szbase/Status/Meaner3/program_uruchomiony" % prefix
			self.reqs[prefix] = self.LADRequest()
			self.reqs[prefix].fr = self.fam.monitorDirectory(dir, prefix)
			params = sn.SZARPDIR + "/%s/config/params.xml" % prefix
			self.reqs[prefix].frp = self.fam.monitorFile(params, prefix)
			try:
				self.reqs[prefix].tests = sn.get_test_params(prefix)
			except SAXParseException:
				continue
			try:
				self.reqs[prefix].max_delay = int(config.get(prefix, 'max_delay'))
			except ConfigParser.NoSectionError, e:
				pass
			except ConfigParser.NoOptionError, e:
				pass
		self.notifyNagios(time.time(), all = True)
		self.first = True

	def closeRequests(self):
		"""
		Reset all requests and close connection to FAM. May be neccesarry after new configurations
		changed.
		"""
		for req in self.reqs.values():
			req.fr.cancelMonitor()
			req.frp.cancelMonitor()
		self.reqs = {}
		self.fam.close()
		self.fam = None
		self.changed.clear()
		self.toreload.clear()
		
	def reloadChanged(self):
		"""
		Reload all changed configurations
		"""
		for prefix in self.toreload:
			logging.debug("reloading configuration for prefix: %s", prefix)
			try:
				self.reqs[prefix].tests = sn.get_test_params(prefix)
			except SAXParseException:
				pass
		self.toreload.clear()

	def checkLast(self, now):
		"""
		Check for configurations that were not updated in time 
		"""
		for prefix in self.reqs.keys():
			if prefix in self.changed:
				continue
			if self.reqs[prefix].failed:
				# do not re-check failed configurations
				continue
			if (now - self.reqs[prefix].last) > self.reqs[prefix].max_delay * 60:
				self.changed.add(prefix)

	def run(self):
		"""
		Run server
		"""
		global sighup
		global sigterm
		self.initRequests()
		last = time.time()
		while True:
			if sighup:
				logging.info("reloading configuration on SIGHUP signal")
				self.initRequests()
				sighup = None
			if sigterm:
				logging.info("exiting on SIGTERM signal")
				sys.exit(0)
			try:
				ri, ro, re = select.select([self.fam], [], [], self.timeout)
			except select.error, er:
				errnumber, strerr = er
				if errnumber == errno.EINTR:
					continue
				else:
					print strerr
					sys.exit(1)
			now = time.time()
			while self.fam.pending():
				fe = self.fam.nextEvent()
				if not self.first:
					self.processEvent(fe, now)
			if self.first:
				self.first = False
			if now - last > self.timeout:
				last = now
				self.reloadChanged()
				self.checkLast(now)
				self.notifyNagios(now)

def handle_sighup(signum, frame):
	"""
	SIGHUP handler - set sighup to reload config
	"""
	global sighup
	signal.signal(signal.SIGHUP, handle_sighup)
	sighup = 1

def handle_sighup_first(signum, fame):
	"""
	first-time SIGHUP handler - does nothing because it's pty.fork()
	that causes this signal
	"""
	signal.signal(signal.SIGHUP, handle_sighup)

def handle_sigterm(signum, frame):
	"""
	SIGINT handler - set setint to exit program
	"""
	global sigterm
	signal.signal(signal.SIGTERM, handle_sigterm)
	sigterm = 1

# command-line handling
usage = "%prog [options]\n%prog watches changes in SZARP configuration and database directories,\nprints notifications in form of Nagios external commands."
parser = OptionParser(usage=usage)
parser.add_option("-n", "--no-fork", dest="fork", action="store_false", default=True,
		help="do not fork and go into background, print command to stdout instead of nagios command file, usefull for debugging")
parser.add_option("-p", "--pid-file", dest="pidfile", default=None, metavar="PIDFILE",
		help="save pid of daemon to PIDFILE, do not start if another copy of daemon is already running")
parser.add_option("-c", "--command-file", dest="cmdfile", metavar="CMDFILE",
		help="print commands to nagios command file CMDFILE")
parser.add_option("-t", "--timeout", dest="timeout", metavar="TIMEOUT", type="int", default=30,
		help="notify nagios not later then TIMEOUT seconds after event, default is %default")
parser.add_option("-f", "--config-file", dest="conffile", metavar="CONFFILE", default="/etc/szarp/szarp-nagios.cfg",
		help="read configuration from CONFFILE, it should contain section [Main] with parameter 'ignored' - space separated list of ignored SZARP configurations; optional 'max_delay' in minutes may be specified in section with name of host; default path to file is %default")
parser.add_option("-l", "--log", dest="logfile", metavar="LOGFILE",
		help="print log informations to LOGFILE")
parser.add_option("-d", "--debug", dest="debug", action="store_true", default=False,
		help="output debug informations to log")
(options, args) = parser.parse_args()
if options.cmdfile is None and options.fork:
	parser.error("-c or -n option is required")
if options.debug and options.logfile is None:
	parser.error("option -d requires -l")


# go into background
if options.fork:
	# we ignore first SIGHUP signal - it's caused by 'fork'
	signal.signal(signal.SIGHUP, handle_sighup_first)
	(pid, fd) = pty.fork()
	if pid != 0:
		# parent
		sys.exit(0)

# global variables for signal handling functions
sighup = None
sigterm = None
signal.signal(signal.SIGTERM, handle_sigterm)

# initialize logging
if options.logfile:
	logger = logging.getLogger()
	if options.debug:
		logger.setLevel(logging.DEBUG)
	else:
		logger.setLevel(logging.INFO)
	lh = logging.handlers.TimedRotatingFileHandler(options.logfile, when="D", interval=1, backupCount="5")
	lh.setFormatter(logging.Formatter("%(asctime)s %(name)s %(levelname)s[%(process)d]: %(message)s"))
	logger.addHandler(lh)

# create pid file
if options.pidfile:
	try:
		# check for other process copies
		pid = int(open(options.pidfile).read().strip())
		os.kill(pid, 0)
		logging.error("another copy of daemon exist (pid %d), exiting", pid)
		sys.exit(1)
	except Exception, e:
		pass
	pidfile = open(options.pidfile, "w")
	pidfile.write("%d\n" % os.getpid())
	pidfile.close()

# open nagios command file
if options.cmdfile is not None:
	logging.info("opening command-file: '%s'", options.cmdfile)
	cmdfile = open(options.cmdfile, "a")
else:
	logging.debug("opening command-file: stdout")
	cmdfile = sys.stdout

# start monitor
try:
	monitor = SzarpMonitor(cmdfile, options.timeout, options.conffile)
	logging.info("starting monitor")
	monitor.run()
except SystemExit, e:
	# called on sys.exit()
	raise e
except BaseException, e:
	# uknown exception occured - log it and raise again
	logging.error("exception: %s", traceback.format_exc())
	raise e
finally:
	if options.pidfile is not None:
		os.unlink(options.pidfile)

