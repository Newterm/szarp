#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

from libipk.plugin import Plugin

from lxml import etree

class CountSubTag( Plugin ) :
	'''counts specified sub tag in param tag'''
	def __init__( self ) :
		Plugin.__init__( self , '//ipk:param' )
		self.params = {} 

	def get_args( self ) :
		return [ 'tagname' ]

	def process( self , node , **args ) :
		exists = False
		try :
			tagname = args['tagname']
		except KeyError :
			raise ValueError('MissingSubTag require tag name argument')
		if node not in self.params :
			self.params[ node ] = 0
		tag = '{' + node.nsmap[None] + '}' + tagname
		for child in node :
			if child.tag == tag :
				exists = True
				break
		if exists :
			self.params[ node ] += 1

	def result( self ) :
		return self.params

	def result_pretty( self ) :
		out = ''
		for p in self.params :
			out += '%i  :  %s\n' % ( self.params[p] , p.get('name') )
		return out[:-1]

class MissingSubTag( CountSubTag ) :
	'''lists param tags with no specified sub tag'''
	def result( self ) :
		return [ k for k in self.params if self.params[k] == 0 ]

	def result_pretty( self ) :
		out = ''
		for p in self.params :
			if self.params[p] == 0 :
				out += p.get('name') + '\n'
		return out[:-1]


