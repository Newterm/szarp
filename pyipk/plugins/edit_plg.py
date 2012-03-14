from libipk.plugin import Plugin

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

	def process( self , root ) :
		self.root = root
		self.num = self.beg
		for node in root.xpath( './/default:%s' % self.tag , namespaces = { 'default' : root.nsmap[None] } ) :
			node.set(self.attrib,'%d'%self.num)
			self.num+=1

	def result( self ) :
		return [ self.root ]


class SortTagAttrib( Plugin ) :
	def set_args( self , **args ) :
		self.attrib = args['attrib']
		self.root   = None

	@staticmethod
	def get_args() :
		return ['attrib']

	def process( self , root ) :
		self.root = root
		root[:] = sorted(root,key=lambda n:n.get(self.attrib))

	def result( self ) :
		return [ self.root ] if self.root != None else []


