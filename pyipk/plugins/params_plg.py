#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

from libipk.plugin import Plugin
from lxml import etree

import unicodedata

import re

class RenameParamName( Plugin ) :
	'''Renames specified sections of param.

	Sets respectively 3 sections for parameter name in pattern "section 1:section 2:section 3". If section is not set its leaved not changed.
	'''
	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.nodes = []

	@staticmethod
	def section() : return 'Param'

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

class FindLswMsw( FindSubStrings ) :
	'''Finds params with names that are substrings of other params names.

	This is also useful while searching for lsw/msw parameters.
	'''

	@staticmethod
	def section() : return 'Param'

	def set_args( self , **args ) :
		FindSubStrings.set_args( self , tag='param' , attrib = 'name' )

	@staticmethod
	def get_args() :
		return []

class LetterChecker( Plugin ) :
	'''Finds params which sections differs but doesn't differ after converting from unicode to ascii

	  * correct to - if not specified only shows nodes which differs, if set will change all sections which doesn't differ after converting to ascii to specified value
	'''
	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.lnodes = ( {} , {} , {} )
		self.cnodes = []

	@staticmethod
	def section() : return 'Param'

	@staticmethod
	def get_args() :
		return [ 'correct to' ]

	def set_args( self , **args ) :
		self.correct = args['correct to']
		self.attrib = 'name'
		self.xpath = ".//ns:%s[@%s]" % ( 'param' , 'name' )

	def normalize_string( self , string ) :
		return unicodedata.normalize('NFKD', unicode(string)).replace(u'ł', 'l').replace(u'Ł', 'L').encode('ascii','ignore')

	def get_sections( self , name ) :
		return name.split(':',3)

	def check( self , a , b ) :
		return self.normalize_string(a) == self.normalize_string(b)

	def process( self , root ) :
		if self.correct == '' :
			self.process_list( root )
		else :
			self.process_correct( root )

	def result( self ) :
		if self.correct == '' :
			return self.result_list() 
		else :
			return self.result_correct()
		
	def process_list( self , root ) :
		for node in root.xpath( self.xpath , namespaces = { 'ns' : root.nsmap[None] } ) :
			secs = self.get_sections( node.attrib[self.attrib] )
			for i in range(3) :
				value = self.normalize_string( secs[i] )
				if value not in self.lnodes[i] :
					self.lnodes[i][ value ]  = []

				append = True
				for n in self.lnodes[i][value] :
					if self.get_sections( n.attrib[self.attrib] )[i] == secs[i] :
						append = False
						break
				if append :
					self.lnodes[i][ value ].append( node )

	def result_list( self ) :
		return reduce( lambda x , y : reduce( lambda a,b : a+b if len(b)>1 else a , y.values() , [] ) + x , self.lnodes , [] )

	def process_correct( self , root ) :
		correct_normal = self.normalize_string( self.correct )
		for node in root.xpath( self.xpath , namespaces = { 'ns' : root.nsmap[None] } ) :
			secs = self.get_sections( node.get(self.attrib) )
			replace = False
			for i in range(3) :
				value = self.normalize_string( secs[i] )
				if value == correct_normal and secs[i] != self.correct :
					secs[i] = self.correct
					replace = True
			if replace :
				node.set( self.attrib , ':'.join(secs) )
				self.cnodes.append( node )
	
	def result_correct( self ) :
		return self.cnodes

