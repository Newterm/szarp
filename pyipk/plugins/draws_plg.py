#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

from libipk.plugin import Plugin
from lxml import etree
import operator as op

from functools import reduce

class DrawsNumbering( Plugin ) :
	'''Corrects all draws numbering beneath selected nodes '''

	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.drawsmap = {} 

	@staticmethod
	def section() : return 'Params'

	def process( self , root ) :
		for draw in root.xpath( './/default:draw' , namespaces = { 'default' : root.nsmap[None] } ) :
			drawtitle = draw.get('title') 
			if drawtitle not in self.drawsmap :
				self.drawsmap[drawtitle] = []

			self.drawsmap[drawtitle].append( draw )

	def result( self ) :
		def tofloat( string ) :
			try :
				return float(string)
			except TypeError :
				return float('inf')

		for draws in self.drawsmap.values() :
			draws.sort( key = lambda d : tofloat(d.get('order',default='inf')) )
			i = 1
			for d in draws :
				d.set('order',str(i))
				i+=3

		drawslist = list(self.drawsmap.values())

		minprior = float('inf')
		for draws in drawslist :
			# find min priority in draws
			prior = float('inf')
			for d in draws :
				prior = min( prior, tofloat(d.get('prior',default='inf')))

			# find min priority in drawslist
			minprior = min(minprior,prior)

			# remove other priorities
			for d in draws :
				if 'prior' in d.attrib :
					d.attrib.pop('prior')

			# set prior on first element
			draws[0].set('prior',str(prior))

		drawslist.sort( key = lambda ds : tofloat(ds[0].get('prior',default='inf')) )

		i = int(minprior) if minprior != float('inf') else 1
		for draws in drawslist :
			draws[0].set('prior',str(i))
			i+=3

		return reduce( lambda a , b : a+b , drawslist , [] )

class DrawsNumberingSpecified( DrawsNumbering ) :
	'''Corrects all draws with specified title numeration beneath selected nodes '''

	@staticmethod
	def get_args() :
		return [ 'drawtitle' ]

	def set_args( self , **args ) :
		self.drawtitle = args['drawtitle']

	def process( self , root ) :
		for draw in root.xpath( './/default:draw[@title="%s"]' % self.drawtitle , namespaces = { 'default' : root.nsmap[None] } ) :
			drawtitle = draw.get('title') 
			if drawtitle not in self.drawsmap :
				self.drawsmap[drawtitle] = []

			self.drawsmap[drawtitle].append( draw )


class DrawsColorsCheck( Plugin ) :
	'''Checks if there are repeated colors in draws if so shows those draws. If drawtitle is empty checks all draws each one separately. '''

	def set_args( self , **args ) :
		self.drawtitle = args['drawtitle']
		self.tags      = {}

	@staticmethod
	def section() : return 'Params'

	@staticmethod
	def get_args() :
		return ['drawtitle']

	def process( self , root ) :
		if self.drawtitle != '' :
			self.process_single( root , self.drawtitle )
		else :
			self.process_all( root )

	def process_single( self , root , title ) :
		for node in root.xpath( './/ns:draw[@title="%s"]' % (title) , namespaces = { 'ns' : root.nsmap[None] } ) :
			self.append_tc( title , node.get('color',default='') , node )

	def process_all( self , root ) :
		for node in root.xpath( './/ns:draw' , namespaces = { 'ns' : root.nsmap[None] } ) :
			self.append_tc( node.get('title') , node.get('color',default='') , node )

	def append_tc( self , title , color , node ) :
		if title in self.tags :
			if color in self.tags[title] :
				self.tags[title][color].append( node )
			else :
				self.tags[title][color] = [node]
		else :
			self.tags[title] = { color : [node] }

	def result( self ) :
		return reduce(
			lambda a , b :
				a + (reduce( op.add , b.values() , [] )
					if reduce(
						lambda r , i :
							r or i[0] != '' and len(i[1]) > 1 ,
						b.items() ,
						False )
					else []) ,
			self.tags.values() ,
			[] )

class PurgeSubTag( Plugin ) :
	'''Removes all tags specified by subtag that are children of tag spec by tag '''
	def set_args( self , **args ) :
		self.tag    = args['tag']
		self.subtag = args['subtag']
		self.tags   = []

	@staticmethod
	def get_args() :
		return ['tag','subtag']

	def process( self , root ) :
		for node in root.xpath( './/ns:%s/ns:%s' % (self.tag,self.subtag) , namespaces = { 'ns' : root.nsmap[None] } ) :
			self.tags.append(node.getparent())
			node.getparent().remove(node)
		if root.tag == '{%s}%s' % (root.nsmap[root.prefix],self.tag) :
			for node in root.xpath( './ns:%s' % self.subtag , namespaces = { 'ns' : root.nsmap[None] } ) :
				self.tags.append(node.getparent())
				node.getparent().remove(node)

	def result( self ) :
		return self.tags

class PurgeDraws( PurgeSubTag ) :
	'''Removes all draws from param tags beneath selected nodes '''
	@staticmethod
	def section() : return 'Params'

	@staticmethod
	def get_args() : return []

	def set_args( self , **args ) :
		PurgeSubTag.set_args( self, tag='param', subtag='draw', **args )

class PurgeReports( PurgeSubTag ) :
	'''Removes all reports from param tags beneath selected nodes '''

	@staticmethod
	def section() : return 'Params'

	@staticmethod
	def get_args() : return []

	def set_args( self , **args ) :
		PurgeSubTag.set_args( self, tag='param', subtag='raport', **args )


class AddDraw( Plugin ) :
	'''Adds all param tags beneath selected nodes to draw '''

	@staticmethod
	def section() : return 'Params'

	@staticmethod
	def get_args() : return ['title']

	def set_args( self , **args ) :
		self.title = args['title']
		self.i = 1
		self.tags = []

	def process( self , root ) :
		def create_draw( title , num , nsmap ) :
			draw = etree.Element( '{%s}draw' % nsmap[None] , nsmap = nsmap )
			draw.set('title',title)
			draw.set('order',str(num))
			return draw

		for node in root.xpath( './/ns:%s' % ('param') , namespaces = { 'ns' : root.nsmap[None] } ) :
			node.append( create_draw(self.title,self.i,root.nsmap) )
			self.tags.append(node)
			self.i+=3

		if root.tag == '{%s}%s' % (root.nsmap[root.prefix],'param') :
			root.append( create_draw(self.title,self.i,root.nsmap) )
			self.tags.append(root)
			self.i+=3

	def result( self ) :
		return self.tags

