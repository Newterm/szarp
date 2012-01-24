#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import lxml

from libipk import errors

from lxml import etree

class Params :
	def __init__( self , filename = None , mode = None , namespaces = None ) :
		self.filename = filename
		self.changed = False
		self.doc = None
		self.nsmap = {}
		if filename :
			self.open( filename , mode , namespaces )

	def open( self , filename , mode = None , namespaces = None ) :
		if self.doc :
			raise IOError('File already opened')
		self.doc = etree.parse( filename )
		self.root = self.doc.getroot()
		self.changed = False

	def touch( self ) :
		self.changed = True

	def save( self , fn = None ) :
		if fn == None : fn = self.filename
		self.doc.write( fn , pretty_print=True )
		self.changed = False

	def close( self ) :
		return not self.changed 

