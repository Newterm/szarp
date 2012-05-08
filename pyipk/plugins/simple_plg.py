#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

from libipk.plugin import Plugin
from lxml import etree

class Dummy( Plugin ) :
	''' Does nothing '''

	def set_args( self , **args ) :
		self.tags = []

	def process( self , root ) :
		self.tags.append( root )

	def result( self ) :
		return self.tags

class ShowParents( Plugin ) :
	'''Shows parents of selected nodes '''

	def set_args( self , **args ) :
		self.tags = []

	def process( self , root ) :
		if root.getparent() not in self.tags :
			self.tags.append( root.getparent() )

	def result( self ) :
		return self.tags

