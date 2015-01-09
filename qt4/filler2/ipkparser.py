# -*- coding: utf-8 -*-

import os
import re
import sys
import csv
import copy
import fnmatch
import operator
import datetime
import calendar
import xml.sax
from collections import namedtuple
from dateutil.tz import tzlocal

sys.path.insert(0, '/opt/szarp/lib')
import libpyszbase as pysz


INF = sys.maxint
LSWMSW_REX = re.compile(r'^\s*\([^:]+:[^:]+:[^:]+ msw\)'
						r'\s*\([^:]+:[^:]+:[^:]+ lsw\)\s*:\s*$')

SZCONFIG_PATH = "/opt/szarp/%s/config/params.xml"
DEFAULT_PATH = "/etc/szarp/default/config/params.xml"

ParamInfo = namedtuple('ParamInfo', 'name draw_name prec lswmsw')
ChangeInfo = namedtuple('ChangeInfo', 'name draw_name set_name user date vals')

class SetInfo:
	__init__ = lambda self, **kw: setattr(self, '__dict__', kw)

class IPKParser:
	def __init__(self, prefix):
		if prefix is None:
			filename = DEFAULT_PATH
		else:
			filename = SZCONFIG_PATH % prefix

		try:
			with open(filename) as fd:
				xml_parser = xml.sax.make_parser()
				handler = IPKParser.IPKDrawSetsHandler()
				xml_parser.setContentHandler(handler)
				xml_parser.setFeature(xml.sax.handler.feature_namespaces, True)
				xml_parser.parse(fd)
				pysz.init("/opt/szarp", u"pl_PL")
				self.ipk_path = os.path.abspath(filename)
				self.ipk_dir = os.path.dirname(os.path.dirname(filename))
				self.ipk_prefix = prefix
				self.ipk_conf = handler.sets
				self.ipk_title = handler.title
		except IOError as err:
			err.bad_path = filename
			raise

	def getTitle(self):
		return self.ipk_title

	def getSets(self):
		slist = []
		for i in self.ipk_conf.items():
			slist.append((i[0], i[1].prior))
		sorted_slist = sorted(slist, key=operator.itemgetter(1))
		return [i[0] for i in sorted_slist]

	def getParams(self, iset):
		return self.ipk_conf[iset].params

	def getDrawSetName(self, pname):
		for key, val in self.ipk_conf.iteritems():
			for p in val.params:
				if p.name == pname:
					return (p.draw_name, key)
		raise ValueError

	def readSZChanges(self):
		szfs = []
		for root, dirnames, filenames in os.walk(os.path.join(self.ipk_dir, 'szbase')):
			for fn in fnmatch.filter(filenames, '*.szf'):
				pname, vals = self.readSZF(os.path.join(root, fn))
				filename, fext = os.path.splitext(fn)
				if fext != '.szf':
					raise ValueError
				date = datetime.datetime.strptime(filename[0:14], "%Y%m%d_%H%M_")
				user = filename.split('_')[2]
				draw_name, set_name = self.getDrawSetName(pname)
				szfs.append(ChangeInfo(pname, draw_name, set_name,
									   user, date, vals))
		return szfs

	def readSZF(self, filename):
		with open(filename) as fd:
			reader = csv.reader(fd, delimiter=',')

			fst_row = reader.next()
			pname = fst_row[1].lstrip().decode('utf-8')
			vals = []

			for row in reader:
				try:
					vals.append(
							(datetime.datetime.strptime(row[0], "%Y-%m-%d %H:%M"),
							 float(row[1]))
							)
				except ValueError:
					vals.append(
							(datetime.datetime.strptime(row[0], "%Y-%m-%d %H:%M"),
							 float('Nan'))
							)

		return (pname, vals)

	# end of readSZF()

	def extrszb10(self, pname, date_list):
		param = "%s:%s" % (self.ipk_prefix, pname)

		out = []
		for d in date_list:
			dt = d.replace(tzinfo=tzlocal())
			t = calendar.timegm(dt.utctimetuple())
			out.append(pysz.get_value(param, t, pysz.PROBE_TYPE.PT_MIN10))

		return out

	class IPKDrawSetsHandler(xml.sax.ContentHandler):
		def __init__(self):
			xml.sax.ContentHandler.__init__(self)
			self.sets = dict()
			self.state = None

			self.param_name = None
			self.param_draw_name = None
			self.param_prec = None
			self.param_lswmsw = None

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
					self.param_name = copy.copy(attrs.getValueByQName('name'))
					try:
						self.param_draw_name = copy.copy(attrs.getValueByQName('draw_name'))
					except KeyError:
						self.param_draw_name = None
					try:
						self.param_prec = copy.copy(attrs.getValueByQName('prec'))
					except KeyError:
						self.param_prec = None
					self.param_lswmsw = None

				elif name == 'define' and self.state == 'DRD':
					if attrs.getValueByQName('type') == "DRAWDEFINABLE" and \
					   LSWMSW_REX.match(attrs.getValueByQName('formula')):
							self.param_lswmsw = True

				elif name == 'draw':
					if self.state == 'DEV' or \
					   (self.state == 'DRD' and self.param_lswmsw == True):
						try:
							prior = int(attrs.getValueByQName('prior'))
						except KeyError, ValueError:
							prior = INF

						if attrs.getValueByQName('title') not in self.sets:
							self.sets[attrs.getValueByQName('title')] = \
									SetInfo(params=[], prior=prior)

						if self.param_draw_name is not None:
							if prior != INF:
								self.sets[attrs.getValueByQName('title')].prior = prior

							self.sets[attrs.getValueByQName('title')].params.append(
									ParamInfo(self.param_name, self.param_draw_name,
									 self.param_prec, self.param_lswmsw))

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

# end of IPKParser class

