#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

from libipk.plugin import Plugin

from lxml import etree

class CountSubTag( Plugin ) :
	'''counts specified sub tag in param tag'''
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
	'''lists param tags with no specified sub tag'''
	def set_args( self , **args ) :
		CountSubTag.set_args( self , count = 0 , **args )

	@staticmethod
	def get_args() :
		args = CountSubTag.get_args()
		args.remove('count')
		return args

