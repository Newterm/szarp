# -*- coding: utf-8 -*-
"""
IPKParser is a module for reading and writing from/to SZARP databases.
"""

# IPKParser is a part of SZARP SCADA software.
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.

__author__    = "Tomasz Pieczerak <tph AT newterm.pl>"
__copyright__ = "Copyright (C) 2014-2015 Newterm"
__version__   = "1.0"
__status__    = "beta"
__email__     = "coders AT newterm.pl"


# imports
import os
import re
import sys
import csv
import copy
import getpass
import fnmatch
import operator
import datetime
import calendar
import xmlrpclib
import subprocess
import xml.sax
from collections import namedtuple
from dateutil.tz import tzlocal

# szarp imports
sys.path.insert(0, '/opt/szarp/lib')
import libpyszbase as pysz

# constants
INF = sys.maxint
SZB_LIMIT = 32767.0
SZB_LIMIT_COM = 2147483647.0
NO_DATA = -32768.0
NO_DATA_COM = -2147483648.0

SZCONFIG_PATH = "/opt/szarp/%s/config/params.xml"
PREFIX_REX = re.compile(r'^[a-z]+$')
LSWMSW_REX = re.compile(r'^\s*\([^:]+:[^:]+:[^:]+ msw\)'
						r'\s*\([^:]+:[^:]+:[^:]+ lsw\)\s*:\s*$')
REMARKS_ADDRESS = "https://eos.newterm.pl:7998/"

# helper types definitions
ParamInfo = namedtuple('ParamInfo', 'name draw_name prec lswmsw')
ChangeInfo = namedtuple('ChangeInfo',
						'name draw_name set_name user date vals lswmsw')
class SetInfo:
	__init__ = lambda self, **kw: setattr(self, '__dict__', kw)


class IPKParser:
	"""SZARP IPKParser is an utility for getting information about SZARP
	   configuration. It acts as typical parser class.
	"""
	class SzbWriterError(Exception):
		"Error raised by szbWriter() method."
		def __init__(self, origin):
			if origin is not None:
				super(Exception, self).__init__(origin.message)
			else:
				super(Exception, self).__init__()

			self.origin = origin
	# end of class SzbWriterError

	class SzfRecordError(Exception):
		"Error raised by recordSzf() method."
		def __init__(self, origin):
			if origin is not None:
				super(Exception, self).__init__(origin.message)
			else:
				super(Exception, self).__init__()

			self.origin = origin
	# end of class SzfRecordError

	def __init__(self, prefix):
		"""IPKParser constructor.

		Arguments:
			prefix - configuration's prefix.
		"""
		if prefix is None or len(prefix) == 0:
			try:
				process = subprocess.Popen(
						["/opt/szarp/bin/lpparse", "prefix"],
						stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				out, err = process.communicate()
				if len(err) == 0:
					prefix = out[1:-1]
			except OSError:
				pass

		if prefix is None or not PREFIX_REX.match(prefix):
			raise ValueError("Non-valid database prefix.")

		# construct SZARP configuration path
		filename = SZCONFIG_PATH % prefix

		try:
			with open(filename) as fd:
				# parse requested configuration
				xml_parser = xml.sax.make_parser()
				handler = IPKParser.IPKDrawSetsHandler()
				xml_parser.setContentHandler(handler)
				xml_parser.setFeature(xml.sax.handler.feature_namespaces, True)
				xml_parser.parse(fd)

				# store all important information
				self.ipk_path = os.path.abspath(filename)
				self.ipk_dir = os.path.dirname(os.path.dirname(filename))
				self.ipk_prefix = prefix
				self.ipk_conf = handler.sets
				self.ipk_title = handler.title

				self.remarks_srv = None
				self.remarks_login = None
		except IOError as err:
			err.bad_path = filename
			raise

	# end of __init__()

	def getTitle(self):
		"""Get configuration's title as fetched from <params> element.

		Returns:
			ipk_title - configuration's title string
		"""
		return self.ipk_title

	def getSets(self):
		"""Get list of sets in configuration, sorted by 'prior' attribute.

		Returns:
			slist - list of sets' titles.
		"""
		slist = []
		for i in self.ipk_conf.items():
			slist.append((i[0], i[1].prior))
		sorted_slist = sorted(slist, key=operator.itemgetter(1))

		return [i[0] for i in sorted_slist]

	# end of getSets()

	def getParams(self, iset):
		"""Get list of parameters in a given set.

		Arguments:
			iset - title of a set as returned by getSets().

		Returns:
			params - list of ParamInfo tuples.
		"""
		return self.ipk_conf[iset].params

	def getSetAndDrawName(self, pname):
		"""Get set title and draw name of a given parameter.

		Arguments:
			pname - full parameter name string.

		Returns:
			(draw_name, set_title) - tuple containing two strings, of draw name
				and set title.
		"""
		# FIXME: set may be ambiguous as parameter may belong to more than one set.
		for set_name, set_info in self.ipk_conf.iteritems():
			for p in set_info.params:
				if p.name == pname:
					return (p.draw_name, set_name)
		raise ValueError

	# end of getSetAndDrawName()

	def readSzfRecords(self):
		"""Search for SZF changes records in the current prefix database.

		Returns:
			[ChangeInfo...] - a list of changes info tuples.
		"""
		szfs = []
		for root, dirnames, filenames in os.walk(os.path.join(self.ipk_dir, 'szbase')):
			for fn in fnmatch.filter(filenames, '*.szf'):
				try:
					pname, vals = self.readSzf(os.path.join(root, fn))
				except:
					continue
				filename, fext = os.path.splitext(fn)
				if fext != '.szf':
					raise ValueError
				if root.rstrip('/')[-4:] == '_lsw':
					lswmsw = True
				else:
					lswmsw = False
				date = datetime.datetime.strptime(filename[0:16], "%Y%m%d_%H%M%S_")
				user = filename.split('_')[2]
				draw_name, set_name = self.getSetAndDrawName(pname)
				szfs.append(ChangeInfo(pname, draw_name, set_name,
									   user, date, vals, lswmsw))
		return szfs

	# end of readSzfRecords()

	def readSzf(self, filepath):
		"""Read SZF change information from a file.

		Arguments:
			filepath - path to SZF file.

		Returns:
			(pname, [(date, value)...]) - a tuple containing full parameter
				name and a list of probe's date and value tuples.
		"""
		with open(filepath) as fd:
			reader = csv.reader(fd, delimiter=',')

			fst_row = reader.next()
			pname = fst_row[1].lstrip().decode('utf-8')
			dvals = []

			for row in reader:
				try:
					dvals.append(
							(datetime.datetime.strptime(row[0], "%Y-%m-%d %H:%M"),
							 float(row[1]))
							)
				except ValueError:
					dvals.append(
							(datetime.datetime.strptime(row[0], "%Y-%m-%d %H:%M"),
							 float('Nan'))
							)

		return (pname, dvals)

	# end of readSzf()

	def extrszb10(self, pname, date_list):
		"""Extract 10-minute probes from SZARP database.

		Arguments:
			pname - full parameter name.
			date_list - list of probes' dates to extract.

		Returns:
			vals - list of extracted probes' values.
		"""
		param = u"%s:%s" % (self.ipk_prefix, pname)
		vals = []

		pysz.init("/opt/szarp", u"pl_PL")
		for d in date_list:
			dt = d.replace(tzinfo=tzlocal())
			t = calendar.timegm(dt.utctimetuple())
			vals.append(pysz.get_value(param, t, pysz.PROBE_TYPE.PT_MIN10))
		pysz.shutdown()

		return vals

	# end of extrszb10()

	def szbWriter(self, pname, dv_list, write_10sec=False):
		"""Write probes' values to SZARP database.

		Arguments:
			pname - full parameter name.
			dv_list - list of probes' tuples (date, value) .
			write_10sec - whether to write also 10-sec probes.
		"""
		try:
			#string = ""
			# FIXME: warm-up fix for szbwriter
			wud = dv_list[0][0] - datetime.timedelta(minutes=10)
			string = u"\"%s\" %s %s\n" % \
					(pname, wud.strftime("%Y %m %d %H %M"), str(NO_DATA))

			for d, v in dv_list:
				string += u"\"%s\" %s %s\n" % \
						(pname, d.strftime("%Y %m %d %H %M"), str(v))
			string = string[:-1]

			cmd = ["/opt/szarp/bin/szbwriter", "-Dprefix=%s" % self.ipk_prefix]
			if not write_10sec:
				cmd.append("-p")
			process = subprocess.Popen(cmd, stdin=subprocess.PIPE,
						stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			out, err = process.communicate(string.encode('utf-8'))
			if process.wait() != 0:
				raise IOError("szbWriter exited with non-zero status")
		except IOError as err:
			raise self.SzbWriterError(err)
		except:
			raise self.SzbWriterError(None)

	# end of szbWriter()

	def pname2path(self, pname, lswmsw):
		"""Convert full parameter name to path in szbase/ directory. For
		combined parameters path to LSW parameter is returned.

		Arguments:
			pname - full parameter name.
			lswmsw - whether parameter is a combined one.

		Returns:
			ppath - parameter's directory path.
		"""
		ppath = "/opt/szarp/%s/szbase/" % self.ipk_prefix
		ppath += self.strip(pname)
		if lswmsw:
			ppath += "_lsw"
		ppath += '/'
		return ppath

	# end of pname2path()

	def strip(self, pname):
		"""Convert full parameter name to parameter's directory name.
		It substitutes all characters but a-zA-Z0-9 with '_'.

		Arguments:
			pname - full parameter name.

		Returns:
			dirname - striped name.
		"""
		def conv(x):
			if   (ord(x) >= ord('a') and ord(x) <= ord('z')) \
			  or (ord(x) >= ord('A') and ord(x) <= ord('Z')) \
			  or (ord(x) >= ord('0') and ord(x) <= ord('9')):
				  return x

			char_conv = {
					u'ę' : u'e', u'ó' : u'o', u'ą' : u'a',
					u'ś' : u's', u'ł' : u'l', u'ż' : u'z',
					u'ź' : u'z', u'ć' : u'c', u'ń' : u'n',
					u'Ę' : u'E', u'Ó' : u'O', u'Ą' : u'A',
					u'Ś' : u'S', u'Ł' : u'L', u'Ż' : u'Z',
					u'Ź' : u'Z', u'Ć' : u'C', u'Ń' : u'N',
					u':' : u'/'
					}

			if x in char_conv:
				return char_conv[x]

			return u"_"

		return "".join([ conv(x) for x in pname ])

	# end of strip()

	def recordSzf(self, pname, dates, lswmsw):
		"""Make a SZF change record for given parameter and probe dates.

		Arguments:
			pname - full parameter name.
			dates - list of probes dates to record.
			lswmsw - whether parameter is a combined one.
		"""
		try:
			output = u""
			vals = self.extrszb10(pname, dates)
			output += u"Data, %s\n" % pname
			for d, v in zip(dates, vals):
				output += u"%s, %s\n" % (d.strftime('%Y-%m-%d %H:%M'), v)

			filepath = u"%s%s_%s.szf" % (self.pname2path(pname, lswmsw),
						datetime.datetime.now().strftime('%Y%m%d_%H%M%S'),
						getpass.getuser())

			with open(filepath, 'w') as fd:
				fd.write(output[:-1].encode('utf-8'))
		except IOError as err:
			raise self.SzfRecordError(err)
		except:
			raise self.SzfRecordError(None)

	# end of recordSzf()

	def initRemarks(self, user, passwd):
		"""Establish a connection to remarks server.

		Argument:
		    user - remarks username.
			pass - MD5 sum of a user's password.
		"""
		try:
			self.remarks_srv = xmlrpclib.Server(REMARKS_ADDRESS)
			self.remarks_login = self.remarks_srv.login(user, passwd)
		except:
			self.remarks_srv = None
			self.remarks_login = None

	# end of initRemarks()

	def postRemark(self, pname, date, remark_title, remark_content):
		"""Post a remark to previously connected server (see iniRemarks()).

		Arguments:
		    pname - full parameter name.
			date - datetime for a remark.
			remark_title - remark's title.
			remark_content - remark's content.
		"""
		if self.remarks_srv is None:
			return # FIXME: better to throw an exception

		# find to which sets parameter belongs
		sets = []
		for set_name, set_info in self.ipk_conf.iteritems():
			for p in set_info.params:
				if p.name == pname:
					sets.append(set_name)

		# calculate epoch time
		dt = date.replace(tzinfo=tzlocal())
		time = calendar.timegm(dt.utctimetuple())

		# add a remark for each set
		for set_name in sets:
			remark = u'<remark prefix="%(prefix)s" ' \
							 'time="%(time)s" ' \
							 'set="%(set_name)s" ' \
							 'title="%(title)s">' \
							 '<content>%(content)s</content></remark>' % \
							 { 'prefix'  : self.ipk_prefix,
							   'time'    : time,
							   'set_name': set_name,
							   'title'   : remark_title,
							   'content' : remark_content }
			remark_bin = xmlrpclib.Binary(remark.encode('utf-8'))
			self.remarks_srv.post_remark(self.remarks_login, remark_bin)

	# end of postRemark()

	def closeRemarks(self):
		"""Close a connection to remarks server."""
		self.remarks_srv = None
		self.remarks_login = None

	# end of closeRemarks()

	class IPKDrawSetsHandler(xml.sax.ContentHandler):
		"""Content handler for SAX with "feature namespaces". Fetches
		information about sets and parameters in the configuration.
		"""
		def __init__(self):
			xml.sax.ContentHandler.__init__(self)
			self.sets = dict()
			self.state = None

			self.param_name = None
			self.param_draw_name = None
			self.param_prec = None
			self.param_lswmsw = None

		# end of __init__()

		def startElementNS(self, name, qname, attrs):
			(uri, name) = name

			if uri == 'http://www.praterm.com.pl/SZARP/ipk':
				if name == 'params':
					self.title = copy.copy(attrs.getValueByQName('title'))
				elif name == 'device':
					self.state = 'DEV'
				elif name == 'defined':
					self.state = 'DEF'
				elif name == 'drawdefinable':
					self.state = 'DRD'

				elif name == 'param':
					# fetch parameter info
					self.param_name = copy.copy(attrs.getValueByQName('name'))
					try:
						self.param_draw_name = copy.copy(attrs.getValueByQName('draw_name'))
					except KeyError:
						self.param_draw_name = None
					try:
						self.param_prec = copy.copy(attrs.getValueByQName('prec'))
					except KeyError:
						self.param_prec = None
					self.param_lswmsw = False

				elif name == 'define' and self.state == 'DRD':
					# lsw/msw parameter from <drawdefinable> section
					if attrs.getValueByQName('type') == "DRAWDEFINABLE" and \
					   LSWMSW_REX.match(attrs.getValueByQName('formula')):
							self.param_lswmsw = True

				elif name == 'draw':
					# visible parameter from <device> or <drawdefinable> (only
					# lsw/msw) section
					if self.state == 'DEV' or \
					   (self.state == 'DRD' and self.param_lswmsw):
						# get 'prior' attribute from <draw>
						try:
							prior = int(attrs.getValueByQName('prior'))
						except KeyError, ValueError:
							prior = INF

						# get 'title' of <draw> and create SetInfo object
						# in dictionary (title is the key)
						if attrs.getValueByQName('title') not in self.sets:
							self.sets[attrs.getValueByQName('title')] = \
									SetInfo(params=[], prior=prior)

						if self.param_draw_name is not None:
							# save set's prior
							if prior != INF:
								self.sets[attrs.getValueByQName('title')].prior = prior

							# create ParamInfo object and store it in appropriate SetInfo
							self.sets[attrs.getValueByQName('title')].params.append(
									ParamInfo(self.param_name, self.param_draw_name,
									 int(self.param_prec), self.param_lswmsw))

		# end of startElementNS()

		def endElementNS(self, name, qname):
			(uri, name) = name

			if uri == 'http://www.praterm.com.pl/SZARP/ipk':
				if name == 'device':
					self.state = None
				elif name == 'defined':
					self.state = None
				elif name == 'drawdefinable':
					self.state = None

				elif name == 'param':
					self.param_name = None
					self.param_draw_name = None
					self.param_prec = None
					self.param_lswmsw = None

		# end of endElementNS()

# end of IPKParser class

