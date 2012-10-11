from libipk.plugin import Plugin

import string , re

from lxml import etree

class EnumerateChoosenAttrib( Plugin ) :
	'''Sets specified attrib to succeding numbers starts from 'start' '''
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

	@staticmethod
	def get_default() :
		return { 'attrib' : 'order' , 'start' : '0' }

	def process( self , node ) :
		self.nodes.append(node)
		node.set(self.attrib,'%d'%self.num)
		self.num += 1

	def result( self ) :
		return self.nodes


class EnumerateSubTagAttrib( Plugin ) :
	'''Sets specified attrib to succeding numbers starts from 'start' '''
	def set_args( self , **args ) :
		self.tag    = args['tag']
		self.beg    = int(args['start'])
		self.attrib = args['attrib']

	@staticmethod
	def get_args() :
		return ['tag','attrib','start']

	@staticmethod
	def get_default() :
		return { 'tag' : 'draw' , 'attrib' : 'order' , 'start' : '0' }

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
	'''Sorts all children of selected tags by specified attrib. Children with no such attrib are grouped at front in undefined order. '''

	def set_args( self , **args ) :
		self.attrib = args['attrib']
		self.roots  = []

	@staticmethod
	def get_args() :
		return ['attrib']

	@staticmethod
	def get_default() :
		return { 'attrib' : 'order' }

	@staticmethod
	def section() :
		return 'Edit'

	def process( self , root ) :
		self.roots.append(root)
		root[:] = sorted(root,key=lambda n:string.lower(n.get(self.attrib,default='')))

	def result( self ) :
		return self.roots

class Search( Plugin ) :
	''' Search and replace string in given tag and attribute.

	If attribute or tag are not provided, searches in all tags and/or attributes.

	It uses regexp to search and replace, so all patterns by python regexp are supported.
	'''

	def set_args( self , **args ) :
		self.tag    = args['tag']
		self.attrib = args['attrib']
		self.search = args['search']
		self.nodes  = []

		self.pattern = re.compile( self.search )

		if self.tag and self.attrib :
			self.xpath = './/df:%s[@%s]' % ( self.tag , self.attrib )
		elif self.tag :
			self.xpath = './/df:%s' % self.tag
		elif self.attrib :
			self.xpath = './/*[@%s]' % self.attrib
		else :
			self.xpath = './/*'

	@staticmethod
	def get_args() :
		return [ 'tag' , 'attrib' , 'search' ]

	def process( self , root ) :
		for node in root.xpath( self.xpath , namespaces = { 'df' : root.nsmap[None] } ) :
			if self.attrib : 
				if self.pattern.search( node.get(self.attrib ) ) :
					self.nodes.append( node )
			else :
				for attrib in node.keys() :
					if self.pattern.search( node.get(attrib ) ) :
						self.nodes.append( node )
						break

	def result( self ) :
		return self.nodes

class SearchAndReplace( Search ) :

	def set_args( self , **args ) :
		Search.set_args( self , **args )
		self.newval = args['replace']

	@staticmethod
	def get_args() :
		return Search.get_args() + [ 'replace' ]

	def replace( self , node , attrib ) :
		val = node.get(attrib)
		res = self.pattern.subn( self.newval , val )
		if res[1] > 0 :
			node.set(attrib,res[0])
		return res[1]

	def process( self , root ) :
		for node in root.xpath( self.xpath , namespaces = { 'df' : root.nsmap[None] } ) :
			num = 0
			if self.attrib : 
				num += self.replace( node , self.attrib )
			else :
				for attrib in node.keys() :
					num += self.replace( node , attrib )

			if num > 0 : self.nodes.append( node )

