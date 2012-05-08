from libipk.plugin import Plugin

import string

from lxml import etree

class EnumerateChoosenAttrib( Plugin ) :
	''' Sets specified attrib to succeding numbers starts from 'start' '''
	def set_args( self , **args ) :
		self.beg    = int(args['start'])
		self.attrib = args['attrib']

		self.num = self.beg
		self.nodes = []

	@staticmethod
	def get_args() :
		return ['attrib','start']

	@staticmethod
	def section() :
		return 'Edit'

	def process( self , node ) :
		self.nodes.append(node)
		node.set(self.attrib,'%d'%self.num)
		self.num += 1

	def result( self ) :
		return self.nodes


class EnumerateSubTagAttrib( Plugin ) :
	''' Sets specified attrib to succeding numbers starts from 'start' '''
	def set_args( self , **args ) :
		self.tag    = args['tag']
		self.beg    = int(args['start'])
		self.attrib = args['attrib']

	@staticmethod
	def get_args() :
		return ['tag','attrib','start']

	@staticmethod
	def section() :
		return 'Edit'

	def process( self , root ) :
		self.root = root
		self.num = self.beg
		for node in root.xpath( './/default:%s' % self.tag , namespaces = { 'default' : root.nsmap[None] } ) :
			node.set(self.attrib,'%d'%self.num)
			self.num+=1

	def result( self ) :
		return [ self.root ]


class SortTagAttrib( Plugin ) :
	''' Sorts all children of selected tags by specified attrib. Children with no such attrib are grouped at front in undefined order. '''

	def set_args( self , **args ) :
		self.attrib = args['attrib']
		self.roots  = []

	@staticmethod
	def get_args() :
		return ['attrib']

	@staticmethod
	def section() :
		return 'Edit'

	def process( self , root ) :
		self.roots.append(root)
		root[:] = sorted(root,key=lambda n:string.lower(n.get(self.attrib,default='')))

	def result( self ) :
		return self.roots


