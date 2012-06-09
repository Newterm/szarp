#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

from libipk.plugin import Plugin
from lxml import etree

import re

class RenameParamName( Plugin ) :
	'''Renames specified sections of param.

	Sets respectively 3 sections for parameter name in pattern "section 1:section 2:section 3". If section is not set its leaved not changed.
	'''
	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.nodes = []

	@staticmethod
	def section() : return 'Params'

	def set_args( self , **args ) :
		self.secs = [ args['section 1'] , args['section 2'] , args['section 3'] ]

	@staticmethod
	def get_args() :
		return [ 'section 1' , 'section 2' , 'section 3' ]

	def process( self , root ) :
		for node in root.xpath( './/ns:param' , namespaces = { 'ns' : root.nsmap[None] } ) :
			secs = node.get('name').split(':')

			for i in range(len(secs)) :
				if i >= len(self.secs) :
					break
				if self.secs[i] != '' :
					secs[i] = self.secs[i]

			node.set('name',':'.join(secs))

			self.nodes.append( node )
	
	def result( self ) :
		return self.nodes

class FindSubStrings( Plugin ) :
	'''Finds tags which name is substring of other tag

	  * tag    - xml tag name
	  * attrib - tag attribute name
	'''
	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.nodes = []

	def set_args( self , **args ) :
		self.attrib = args['attrib']
		self.xpath = ".//ns:%s[@%s]" % ( args['tag'] , args['attrib'] )

	@staticmethod
	def get_args() :
		return [ 'tag' , 'attrib' ]

	def process( self , root ) :
		for node in root.xpath( self.xpath , namespaces = { 'ns' : root.nsmap[None] } ) :
			self.nodes.append( node )
	
	def result( self ) :
		out = []
		self.nodes.sort( key = lambda n : n.get( self.attrib ) )
		i = 0
		while i < len(self.nodes)-1 :
			n0 = self.nodes[i  ]
			s0 = n0.get(self.attrib)
			j = 1
			while i+j < len(self.nodes) :
				n1 = self.nodes[i+j]
				s1 = n1.get(self.attrib)
				if s1.startswith( s0 ) :
					if j == 1 : out.append( n0 )
					out.append( n1 )
				else :
					break
				j+=1
			i += j
		return out

class CheckParamsForISL( FindSubStrings ) :
	'''Finds params with names that are substrings of other params names. Thats sometimes an issue in ISL diagrams.
	'''

	@staticmethod
	def section() : return 'Params'

	def set_args( self , **args ) :
		FindSubStrings.set_args( self , tag='param' , attrib = 'name' )

	@staticmethod
	def get_args() :
		return []

