#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import lxml

from libipk import errors

from lxml import etree

toUtf8 = lambda s: s.decode('utf8')

class PNode :
	def __init__( self , doc , parent , node ) :
		self.doc = doc
		self.parent = parent
		self.node = node
		self.__build_children()

	def __len__( self ) :
		return len(self.children)

	def __getitem__( self, key ) :
		return self.children[key]

	def touch( self ) :
		self.doc.touch()

	def getline( self ) :
		return self.node.sourceline

	def toline( self ) :
#        ns = n.nsmap[None]
#        n.nsmap[None] = ''
#        out = toUtf8( etree.tostring( n , pretty_print = True , encoding = 'utf8' , method = 'xml' ) ).partition('\n')[0]
#        n.nsmap[None] = ns
#        return out
		if isinstance(self.node.tag,str) :
			tag = self.node.tag.replace( '{%s}' % self.node.nsmap[self.node.prefix] , '%s:' % self.node.prefix if self.node.prefix != None else  '' )
			out = '<%s '% tag + ' '.join(['%s="%s"'%(a,self.node.get(a)) for a in self.node.attrib]) + '>'
		else :
			out = toUtf8( etree.tostring( self.node , pretty_print = True , encoding = 'utf8' , method = 'xml' ) ).partition('\self.node')[0]
		return out

	def tostring( self ) :
		return etree.tostring( self.node , pretty_print = True , encoding='utf8' , method = 'xml' , xml_declaration=False )

	def __repr__( self ) :
		return '('+object.__repr__(self)+','+str(self.node)+')'

	def replace( self , lxmlnode ) :
		if self.parent != None :
			self.parent.node.replace( self.node , lxmlnode )
		self.node = lxmlnode
		self.__rebuild_children()

	def remove( self , child ) :
		try :
			self.children.remove( child )
			self.node.remove( child.node )
			child.parent = None
		except ValueError :
			raise ValueError('That child is not mine: ' + self.toline())

	def rebuild( self ) :
		self.__rebuild_children()

	def _remove( self , child ) :
		pass

	def insert( self , pos , child ) :
		if child in self.children :
			return
		self.children.insert( pos , child )
		self.node.insert( pos , child.node )
		child.parent = self
		self.touch()

	def _insert( self , pos , child ) :
		pass

	def __create( self , node ) :
		return self.__class__( self.doc , self , node )

	def __build_children( self ) :
		if self.node == None :
			raise ValueError('Node must be defined')

		self.children = []

		for n in self.node :
			self.children.append( self.__create(n) )

	def _index( self , node , start = 0 ) :
		for i in range(start,len(self.children)) :
			if self.children[i].node == node :
				return i
		return -1

	def __rebuild_children( self ) :
		for i in range(len(self.node)) :
			if i < len(self) and self.children[i].node == self.node[i] :
				continue

			idx = self._index( self.node[i] , i )
			if idx >= 0 :
				self.children[i] , self.children[idx] = \
					self.children[idx] , self.children[i]
			else :
				self.children.insert( i , self.__create(self.node[i]) )

			self.touch()

		if len(self.node) < len(self.children) :
			del self.children[len(self.node):]
			self.touch()

		for c in self.children : c.rebuild()


class Params :
	def __init__( self , filename = None , mode = None , namespaces = None , treeclass = PNode ) :
		self.filename = filename
		self.changed = False
		self.doc = None
		self.treeclass = treeclass
		self.nsmap = {}
		if filename :
			self.open( filename , mode , namespaces , treeclass )

	def open( self , filename , mode = None , namespaces = None , treeclass = PNode ) :
		if self.doc :
			raise IOError('File already opened')
		self.doc = etree.parse( filename )
		self.root = treeclass( self , None , self.doc.getroot() )
		self.changed = False

	def getroot( self ) :
		return self.root

	def touch( self ) :
		self.changed = True

	def save( self , fn = None ) :
		if fn == None : fn = self.filename
		self.doc.write( fn , pretty_print=True , method = 'xml' , xml_declaration=True , encoding=self.doc.docinfo.encoding )
		self.changed = False

	def close( self ) :
		return not self.changed 

	def rebuild_tree( self , nodes = [] ) :
		''' Rebuilds PNode tree to consits with lxml tree.
		
		nodes -- suggests where are canges, but not provide whole information
		returns -- list of PNodes equivalent to lxml nodes 
		'''
		if self.root.node != self.doc.getroot() :
			del self.root
			self.root = self.treeclass( self , None , self.doc.getroot() )

		self.root.rebuild()

		return [ self.get_pnode( self.find_node_path( n ) ) for n in nodes ]

	def find_node_path( self , node ) :
		path = []

		parent = node.getparent()
		while parent != None :
			path.insert( 0 , parent.index( node ) )

			node = parent
			parent = node.getparent()

		if node != self.getroot().node :
			raise ValueError('Node not in my tree')

		return path

	def get_pnode( self , path ) :
		node = self.getroot()
		for p in path :
			try :
				node = node[p]
			except IndexError :
				return None
		return node

