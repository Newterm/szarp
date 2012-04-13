#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

from libipk.plugin import Plugin

from functools import reduce

from lxml import etree

class CountSubTag( Plugin ) :
	'''Shows tags with specified number of subtags

	  * tag    - xml tag name
	  * subtag - xml children tag name
	  * count  - number of repeats
	'''
	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.params = {} 

	def set_args( self , **args ) :
		self.tag    = args['tag']
		self.subtag = args['subtag']
		self.count  = int(args['count'])

	@staticmethod
	def get_args() :
		return [ 'tag' , 'subtag' , 'count' ]

	def process( self , root ) :
		for node in root.xpath( './/default:%s' % self.tag , namespaces = { 'default' : root.nsmap[None] } ) :
			if node not in self.params :
				self.params[ node ] = 0
			tagname = '{' + node.nsmap[None] + '}' + self.subtag
			for child in node :
				if child.tag == tagname :
					self.params[ node ] += 1

	def result( self ) :
		return [ k for k in self.params if self.params[k] == self.count ]

class MissingSubTag( CountSubTag ) :
	'''Lists tags without specified subtags. Same as CountSubTag with count = 0'''
	def set_args( self , **args ) :
		CountSubTag.set_args( self , count = 0 , **args )

	@staticmethod
	def get_args() :
		args = CountSubTag.get_args()
		args.remove('count')
		return args

class XPath( Plugin ) :
	'''Executes xpath and lists all nodes that match this xpath'''
	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.nodes = [] 

	def set_args( self , **args ) :
		self.xpath = args['xpath']

	@staticmethod
	def get_args() :
		return [ 'xpath' ]

	def process( self , root ) :
		for node in root.xpath( self.xpath , namespaces = { 'ns' : root.nsmap[None] } ) :
			self.nodes.append( node )
	
	def result( self ) :
		return self.nodes

class FindAttrib( XPath ) :
	'''Finds tags with attribute same as specified value

	  * tag    - xml tag name
	  * attrib - attribute name
	  * value  - attribute value
	'''
	def __init__( self , **args ) :
		XPath.__init__( self , **args )

	def set_args( self , **args ) :
		self.xpath = ".//ns:%s[@%s='%s']" % ( args['tag'] , args['attrib'] , args['value'] )

	@staticmethod
	def get_args() :
		return [ 'tag' , 'attrib' , 'value' ]

	def result( self ) :
		return list( self.nodes )

class FindRepeated( Plugin ) :
	'''Finds tags with repeated attrib values

	  * tag    - xml tag name
	  * attrib - tag attribute name
	'''
	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.nodes = {}

	def set_args( self , **args ) :
		self.attrib = args['attrib']
		self.xpath = ".//ns:%s[@%s]" % ( args['tag'] , args['attrib'] )

	@staticmethod
	def get_args() :
		return [ 'tag' , 'attrib' ]

	def process( self , root ) :
		for node in root.xpath( self.xpath , namespaces = { 'ns' : root.nsmap[None] } ) :
			value = node.attrib[self.attrib]
			if value not in self.nodes :
				self.nodes[ value ]  = []
			self.nodes[ value ].append( node )
	
	def result( self ) :
		return reduce( lambda a , b : a + b if len(b)>1 else a , self.nodes.values() , [] )

