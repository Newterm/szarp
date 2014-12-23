# -*- coding: utf-8 -*-

import sys
import re
import copy
import operator
import xml.sax
from collections import namedtuple

INF = sys.maxint
LSWMSW_REX = re.compile(r'^\s*\([^:]+:[^:]+:[^:]+ msw\)'
						r'\s*\([^:]+:[^:]+:[^:]+ lsw\)\s*:\s*$')

ParamInfo = namedtuple('ParamInfo', 'name draw_name prec lswmsw')

class SetInfo:
	__init__ = lambda self, **kw: setattr(self, '__dict__', kw)

class IPKParser:
	def __init__(self, filename):
		with open(filename) as fd:
			xml_parser = xml.sax.make_parser()
			handler = IPKParser.IPKDrawSetsHandler()
			xml_parser.setContentHandler(handler)
			xml_parser.setFeature(xml.sax.handler.feature_namespaces, True)
			xml_parser.parse(fd)
			self.ipk_conf = handler.sets
			self.ipk_title = handler.title

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

