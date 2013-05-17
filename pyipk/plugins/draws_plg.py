#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

from libipk.plugin import Plugin
from lxml import etree
import operator as op

from functools import reduce

def tofloat( string ) :
	try :
		return float(string)
	except TypeError :
		return float('inf')

class NormalizeDraws( Plugin ) :
	'''Corrects all draws numbering beneath selected nodes '''

	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.drawsmap = {} 

	@staticmethod
	def section() : return 'Draw'

	@staticmethod
	def get_args() :
		return [ 'start' , 'step' ]

	@staticmethod
	def get_default() :
		return { 'start' : '2' , 'step' : '2' }

	def set_args( self , **args ) :
		self.start = float(args['start'])
		self.step  = float(args['step'])

	def process( self , root ) :
		for draw in root.xpath( './/default:draw' , namespaces = { 'default' : root.nsmap[None] } ) :
			drawtitle = draw.get('title') 
			if drawtitle not in self.drawsmap :
				self.drawsmap[drawtitle] = []

			self.drawsmap[drawtitle].append( draw )

	def result( self ) :
		for draws in self.drawsmap.values() :
			draws.sort( key = lambda d : tofloat(d.get('order',default='inf')) )
			i = self.start 
			for d in draws :
				d.set('order',str(i))
				i+=self.step

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

		i = int(minprior) if minprior != float('inf') else self.start
		for draws in drawslist :
			draws[0].set('prior',str(i))
			i+=self.step

		return reduce( lambda a , b : a+b , drawslist , [] )

class NormalizeDrawsSpecified( NormalizeDraws ) :
	'''Corrects all draws with specified title numeration beneath selected nodes '''

	@staticmethod
	def get_args() :
		return [ 'drawtitle' ] + NormalizeDraws.get_args()

	@staticmethod
	def get_default() :
		return NormalizeDraws.get_default()

	def set_args( self , **args ) :
		self.drawtitle = args['drawtitle']
		NormalizeDraws.set_args( self , **args )

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
	def section() : return 'Draw'

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
	def section() : return 'Draw'

	@staticmethod
	def get_args() : return []

	def set_args( self , **args ) :
		PurgeSubTag.set_args( self, tag='param', subtag='draw', **args )

class PurgeReports( PurgeSubTag ) :
	'''Removes all reports from param tags beneath selected nodes '''

	@staticmethod
	def section() : return 'Report'

	@staticmethod
	def get_args() : return []

	def set_args( self , **args ) :
		PurgeSubTag.set_args( self, tag='param', subtag='raport', **args )


class AddDraw( Plugin ) :
	'''Adds all param tags beneath selected nodes to draw '''

	@staticmethod
	def section() : return 'Draw'

	@staticmethod
	def get_args() :
		return [ 'title' , 'start' , 'step' ]

	@staticmethod
	def get_default() :
		return { 'start' : '2' , 'step' : '2' }

	def set_args( self , **args ) :
		self.i     = float(args['start'])
		self.step  = float(args['step'])
		self.title = args['title']
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
			self.i+=self.step

		if root.tag == '{%s}%s' % (root.nsmap[root.prefix],'param') :
			root.append( create_draw(self.title,self.i,root.nsmap) )
			self.tags.append(root)
			self.i+=self.step

	def result( self ) :
		return self.tags

class ListDraws( Plugin ) :
	''' Lists all draws in order defined by prior attribute. '''

	@staticmethod
	def section() : return 'Draw'

	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.draws = {}

	def process( self , root ) :
		for node in root.xpath( './/default:draw' , namespaces = { 'default' : root.nsmap[None] } ) :
			title = node.get('title')
			if title not in self.draws :
				self.draws[title] = node
			else :
				old = self.draws[title]
				np  = tofloat(node.get('prior',default='inf'))
				op  = tofloat(old .get('prior',default='inf'))
				if np < op :
					self.draws[title] = node

	def result( self ) :
		return sorted( self.draws.values() , key = lambda n : tofloat(n.get('prior')) )

class ApplyDrawsPrior( Plugin ) :
	''' Sets prior to draw tag in specified order. Use with ListDraws. '''

	@staticmethod
	def section() : return 'Draw'

	@staticmethod
	def get_args() :
		return [ 'start' , 'step' ]

	@staticmethod
	def get_default() :
		return { 'start' : '2' , 'step' : '2' }

	def set_args( self , **args ) :
		self.prior = float(args['start'])
		self.step  = float(args['step'])

	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.nodes = []

	def process( self , node ) :
		if node.tag != '{%s}%s' % (node.nsmap[node.prefix],'draw') :
			return
		node.set('prior',str(self.prior))
		self.prior += self.step
		self.nodes.append(node)

	def result( self ) :
		return self.nodes

class ApplyParamsOrder( Plugin ) :
	''' Sets order to draw tag in specified order. Use with ListParams. '''

	@staticmethod
	def section() : return 'Draw'

	@staticmethod
	def get_args() :
		return [ 'title' , 'start' , 'step' ]

	@staticmethod
	def get_default() :
		return { 'start' : '2' , 'step' : '2' }

	def set_args( self , **args ) :
		self.title = args['title']
		self.order = float(args['start'])
		self.start = self.order
		self.step  = float(args['step'])

	def __init__( self , **args ) :
		Plugin.__init__( self , **args )
		self.nodes = []
		self.priorize = None
		self.prior = tofloat('inf')

	def process( self , node ) :
		if node.tag != '{%s}%s' % (node.nsmap[node.prefix],'param') :
			return
		for n in node.xpath( ".//ns:draw[@title='%s']" % self.title , namespaces = { 'ns' : node.nsmap[None] } ) :
			n.set('order',str(self.order))
			if 'prior' in n.attrib :
				self.prior = min(self.prior,tofloat(n.get('prior',default='inf')))
				n.attrib.pop('prior')
			if self.order == self.start :
				self.priorize = n
		self.order += self.step
		self.nodes.append(node)

	def result( self ) :
		if self.priorize != None : self.priorize.set('prior',str(int(self.prior)))
		return self.nodes

