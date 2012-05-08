#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

from libipk.plugin import Plugin
from lxml import etree

import re

class RenameParamName( Plugin ) :
	''' Renames specified sections of param.

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


